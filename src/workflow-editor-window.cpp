#include "workflow-editor-window.h"
#include "workflow-editor-view.h"
#include "workflow-editor-toolbar.h"
#include "workflow-editor-sidebar.h"
#include "workflow-editor-properties.h"
#include "workflow-model.h"
#include "workflow-node-dialog.h"
#include "workflow-node-duplicate-ui.h"
#include "workflow-clipboard.h"
#include "workflow-persistence.h"
#include "workflow-export.h"
#include "workflow-import.h"
#include "workflow-scene.h"
#include "workflow-undo.h"
#include "workflow-workspace.h"
#include "workflow-hotkeys.h"
#include <obs-frontend-api.h>
#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPointer>
#include <QShortcut>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <cstring>

namespace {
class EditorWindow;
static QPointer<EditorWindow> window;

class EditorWindow final : public QDialog {
    Q_OBJECT
public:
    explicit EditorWindow(QWidget *parent=nullptr):QDialog(parent){
        setWindowTitle("Move Workflow Editor");resize(1350,800);
        setStyleSheet("QDialog{background:#0c1218;color:#e6edf3;} QSplitter::handle{background:#27313c;} QLabel{color:#dbe4ec;}");
        auto *root=new QVBoxLayout(this);root->setContentsMargins(6,6,6,6);root->setSpacing(6);
        scene_=new EditorScene(this);view_=new WorkflowGraphicsView(scene_,this);view_->installEventFilter(this);qApp->installEventFilter(this);workspace_.scene=scene_;workflow_workspace_init(&workspace_,scene_);resetUndo();
        workflow_editor_toolbar_callbacks toolbar;
        toolbar.select_workflow=[this](const char *id){if(workflow_workspace_select(&workspace_,id)){resetUndo();QTimer::singleShot(0,this,[this]{view_->fitAll();});refreshUi();}};
        toolbar.create_workflow=[this](const char *name){const bool ok=workflow_workspace_create(&workspace_,name);if(ok){resetUndo();workflow_persistence_sync(workflow_workspace_manager(&workspace_));}return ok;};
        toolbar.duplicate_workflow=[this](const char *name){const bool ok=workflow_workspace_duplicate(&workspace_,name);if(ok){resetUndo();workflow_persistence_sync(workflow_workspace_manager(&workspace_));}return ok;};toolbar.delete_workflow=[this]{return deleteWorkflow();};toolbar.rename_workflow=[this]{renameWorkflow();};
        toolbar.set_workflow_enabled=[this](bool enabled){auto *manager=workflow_workspace_manager(&workspace_);const auto *selected=workflow_manager_selected_const(manager);if(selected){workflow_manager_set_enabled(manager,selected->id,enabled);workflow_persistence_sync(manager);undo_.captureManager();}};
        toolbar.import_workflow=[this]{importWorkflow();};toolbar.export_workflow=[this]{exportWorkflow();};toolbar.zoom_out=[this]{view_->zoomOut();};toolbar.zoom_reset=[this]{view_->resetZoom();};toolbar.zoom_in=[this]{view_->zoomIn();};toolbar.fit=[this]{view_->fitAll();};toolbar.close=[this]{hide();};
        toolbar_=create_workflow_editor_toolbar(this,workflow_workspace_manager(&workspace_),std::move(toolbar));root->addWidget(toolbar_);
        workflow_editor_sidebar_callbacks sidebar;
        sidebar.add_trigger=[this]{addNodeFromPalette("trigger");};sidebar.add_node=[this](const char *kind){addNodeFromPalette(kind);};
        sidebar.select_node=[this](const char *id){if(!id)return;for(auto *node:scene_->nodes())if(node->id()==QString::fromUtf8(id)){scene_->clearSelection();node->setSelected(true);break;}};
        sidebar.edit_node=[this]{editSelectedNode();};sidebar.copy_node=[this]{copySelectedNodes();};sidebar.paste_node=[this]{pasteNodes();};sidebar.duplicate_node=[this]{duplicateSelectedNode();};sidebar.delete_node=[this]{deleteSelectedNodes();};
        sidebar_=create_workflow_editor_sidebar(this,std::move(sidebar));properties_=create_workflow_editor_properties(this,[this](NodeItem *node){editNode(node);});
        auto *splitter=new QSplitter(Qt::Horizontal,this);splitter->addWidget(sidebar_);splitter->addWidget(view_);splitter->addWidget(properties_);splitter->setStretchFactor(1,1);splitter->setSizes({250,850,280});root->addWidget(splitter,1);
        auto *status=new QHBoxLayout;status->setContentsMargins(6,2,6,2);status->addWidget(new QLabel("Workflow canvas",this));status->addStretch();zoomLabel_=new QLabel("100%",this);status->addWidget(new QLabel("Zoom:",this));status->addWidget(zoomLabel_);root->addLayout(status);view_->setZoomLabel(zoomLabel_);
        connect(scene_,&QGraphicsScene::selectionChanged,this,[this]{updateButtonState();});connect(scene_,&EditorScene::nodeDoubleClicked,this,[this](NodeItem *node){editNode(node);});connect(scene_,&QGraphicsScene::changed,this,[this]{scheduleSync();});
        auto *undoShortcut=new QShortcut(QKeySequence::Undo,this);undoShortcut->setContext(Qt::WidgetWithChildrenShortcut);connect(undoShortcut,&QShortcut::activated,this,&EditorWindow::undoWorkflow);auto *deleteShortcut=new QShortcut(QKeySequence::Delete,this);deleteShortcut->setContext(Qt::WidgetWithChildrenShortcut);connect(deleteShortcut,&QShortcut::activated,this,&EditorWindow::deleteSelectedNodes);updateButtonState();
    }
    ~EditorWindow() override{beginShutdown();if(qApp)qApp->removeEventFilter(this);}
public slots:void redoFromHotkey(){if(shuttingDown_)return;blog(LOG_INFO,"[Move Workflow] Ctrl+Y Qt slot reached.");redoWorkflow();}
protected:bool eventFilter(QObject *watched,QEvent *event)override{return QDialog::eventFilter(watched,event);}void keyPressEvent(QKeyEvent *event)override{QDialog::keyPressEvent(event);}
private:
    void beginShutdown(){if(shuttingDown_)return;shuttingDown_=true;if(scene_)QObject::disconnect(scene_,nullptr,this,nullptr);}
    void scheduleSync(){if(shuttingDown_||syncPending_||applyingUndoRedo_)return;syncPending_=true;const auto generation=syncGeneration_;QTimer::singleShot(0,this,[this,generation]{if(shuttingDown_)return;syncPending_=false;if(applyingUndoRedo_||generation!=syncGeneration_)return;workflow_workspace_sync_scene(&workspace_);undo_.capture();updateButtonState();});}
    void resetUndo(){if(applyingUndoRedo_)return;++syncGeneration_;syncPending_=false;undo_.reset(workflow_manager_selected(workflow_workspace_manager(&workspace_)),workflow_workspace_manager(&workspace_));}
    void syncLoadedSelection(){const auto *s=workflow_manager_selected_const(workflow_workspace_manager(&workspace_));if(s){std::strncpy(workspace_.loaded_workflow_id,s->id,WORKFLOW_MAX_NAME-1);workspace_.loaded_workflow_id[WORKFLOW_MAX_NAME-1]='\0';}else workspace_.loaded_workflow_id[0]='\0';}
    void refreshUi(){if(shuttingDown_)return;workflow_editor_toolbar_refresh(toolbar_);updateButtonState();}
    void undoWorkflow(){if(shuttingDown_)return;++syncGeneration_;syncPending_=false;applyingUndoRedo_=true;if(undo_.undo()){syncLoadedSelection();workflow_workspace_reload(&workspace_);workflow_persistence_sync(workflow_workspace_manager(&workspace_));refreshUi();}applyingUndoRedo_=false;}
    void redoWorkflow(){if(shuttingDown_)return;++syncGeneration_;syncPending_=false;applyingUndoRedo_=true;if(undo_.redo()){syncLoadedSelection();workflow_workspace_reload(&workspace_);workflow_persistence_sync(workflow_manager_selected_const(workflow_workspace_manager(&workspace_))?workflow_workspace_manager(&workspace_):workflow_workspace_manager(&workspace_));refreshUi();}applyingUndoRedo_=false;}
    bool deleteWorkflow(){workflow_workspace_sync_scene(&workspace_);auto *manager=workflow_workspace_manager(&workspace_);const auto *selected=workflow_manager_selected_const(manager);if(!selected)return false;undo_.prepareManagerCapture();if(!workflow_manager_remove(manager,selected->id))return false;syncLoadedSelection();if(workspace_.loaded_workflow_id[0])workflow_workspace_reload(&workspace_);else{const QList<NodeItem*>nodes=scene_->nodes();for(NodeItem *node:nodes)scene_->deleteNode(node);}undo_.captureManager();workflow_persistence_sync(manager);refreshUi();return true;}
    void renameWorkflow(){auto *selected=workflow_manager_selected(workflow_workspace_manager(&workspace_));if(!selected)return;bool ok=false;const QString name=QInputDialog::getText(this,"Rename Workflow","Workflow name:",QLineEdit::Normal,QString::fromUtf8(selected->name),&ok);if(ok&&!name.trimmed().isEmpty()){std::snprintf(selected->name,sizeof(selected->name),"%s",name.trimmed().toUtf8().constData());workflow_persistence_sync(workflow_workspace_manager(&workspace_));undo_.captureManager();refreshUi();}}
    void importWorkflow(){const QString file=QFileDialog::getOpenFileName(this,"Import Workflow",{},"Move Workflow (*.obsworkflow.json)");if(file.isEmpty())return;auto *manager=workflow_workspace_manager(&workspace_);if(workflow_import_file(manager,file.toUtf8().constData())){syncLoadedSelection();workflow_workspace_reload(&workspace_);workflow_persistence_sync(manager);resetUndo();refreshUi();view_->fitAll();}}
    void exportWorkflow(){workflow_workspace_sync_scene(&workspace_);const QString file=QFileDialog::getSaveFileName(this,"Export Workflow",{},"Move Workflow (*.obsworkflow.json)");if(!file.isEmpty())workflow_export_selected(workflow_workspace_manager(&workspace_),file.toUtf8().constData());}
    void addNodeFromPalette(const char *kind){bool ok=false;const bool trigger=std::strcmp(kind,"trigger")==0;const QString name=QInputDialog::getText(this,trigger?"Add Trigger Node":"Add Action Node","Node name:",QLineEdit::Normal,trigger?"New Trigger":"New Action",&ok);if(!ok||name.trimmed().isEmpty())return;NodeItem *node=scene_->addNode(trigger?WORKFLOW_NODE_TRIGGER:WORKFLOW_NODE_ACTION,name.trimmed());if(node){node->setSelected(true);view_->fitAll();}}
    void editNode(NodeItem *node){if(!node||shuttingDown_)return;if(edit_node_settings(node,scene_->nodes(),this)){node->refreshDisplay();scene_->refreshConnectionsFor(node);refreshUi();}}
    void editSelectedNode(){editNode(scene_->selectedNode());}void copySelectedNodes(){if(workflow_clipboard_copy(scene_))updateButtonState();}void pasteNodes(){if(workflow_clipboard_paste(scene_))updateButtonState();}void duplicateSelectedNode(){if(duplicate_selected_workflow_node(scene_))updateButtonState();}
    void deleteSelectedNodes(){if(shuttingDown_)return;const QList<QGraphicsItem*>selected=scene_->selectedItems();QList<NodeItem*>nodes;for(QGraphicsItem *item:selected)if(auto *node=dynamic_cast<NodeItem*>(item))nodes.append(node);if(nodes.isEmpty())return;workflow_workspace_sync_scene(&workspace_);for(NodeItem *node:nodes)scene_->deleteNode(node);workflow_workspace_sync_scene(&workspace_);undo_.capture();updateButtonState();}
    void updateButtonState(){if(shuttingDown_||!scene_)return;const bool selected=scene_->selectedNode();workflow_editor_sidebar_set_selection_state(sidebar_,selected,workflow_clipboard_has_data());workflow_editor_sidebar_set_workflow_nodes(sidebar_,scene_->nodes(),selected?scene_->selectedNode():nullptr);workflow_editor_properties_set_node(properties_,selected?scene_->selectedNode():nullptr);}
    workflow_workspace_t workspace_{};WorkflowUndo undo_;EditorScene *scene_=nullptr;WorkflowGraphicsView *view_=nullptr;QWidget *toolbar_=nullptr;QWidget *sidebar_=nullptr;QWidget *properties_=nullptr;QLabel *zoomLabel_=nullptr;bool syncPending_=false;bool applyingUndoRedo_=false;bool shuttingDown_=false;unsigned long long syncGeneration_=0;
};
}
extern "C" void workflow_editor_redo_from_hotkey(void){EditorWindow *editor=window.data();if(!editor){blog(LOG_WARNING,"[Move Workflow] Ctrl+Y bridge fired but editor is not open.");return;}QMetaObject::invokeMethod(editor,"redoFromHotkey",Qt::QueuedConnection);}
void show_move_workflow_editor(QWidget *parent){if(!window){auto *mainWindow=parent?parent:static_cast<QWidget*>(obs_frontend_get_main_window());window=new EditorWindow(mainWindow);}window->show();window->raise();window->activateWindow();}
void destroy_move_workflow_editor(void){if(!window)return;window->close();delete window.data();window=nullptr;}
#include "workflow-editor-window.moc"
