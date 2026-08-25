#include "workflow-editor-window.h"
#include "workflow-editor-view.h"
#include "workflow-manager-ui.h"
#include "workflow-model.h"
#include "workflow-node-dialog.h"
#include "workflow-node-duplicate-ui.h"
#include "workflow-clipboard.h"
#include "workflow-persistence.h"
#include "workflow-scene.h"
#include "workflow-undo.h"
#include "workflow-workspace.h"
#include "workflow-hotkeys.h"
#include "workflow-debug-ui.h"
#include <obs-frontend-api.h>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QEvent>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>
#include <QComboBox>
#include <QDebug>
#include <QAction>
#include <QMetaObject>
#include <cstring>

namespace {
class EditorWindow;
static QPointer<EditorWindow> window;

class EditorWindow final : public QDialog {
    Q_OBJECT
public:
    explicit EditorWindow(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Move Workflow Editor"); resize(1050, 700);
        auto *root = new QVBoxLayout(this); auto *toolbar = new QHBoxLayout;
        addButton_=new QPushButton("+ Add Node",this); auto *edit=new QPushButton("Edit Node",this);
        copyButton_=new QPushButton("Copy",this); pasteButton_=new QPushButton("Paste",this);
        duplicateButton_=new QPushButton("Duplicate Node",this); deleteButton_=new QPushButton("Delete Node",this);
        auto *zoomOut=new QPushButton("−",this); auto *zoomReset=new QPushButton("100%",this); auto *zoomIn=new QPushButton("+",this); auto *fit=new QPushButton("Fit All",this); auto *close=new QPushButton("Close",this);
        toolbar->addWidget(addButton_); toolbar->addWidget(edit); toolbar->addWidget(copyButton_); toolbar->addWidget(pasteButton_); toolbar->addWidget(duplicateButton_); toolbar->addWidget(deleteButton_); toolbar->addStretch(); toolbar->addWidget(workflow_debug_create_control(this)); toolbar->addWidget(zoomOut); toolbar->addWidget(zoomReset); toolbar->addWidget(zoomIn); toolbar->addWidget(fit); toolbar->addWidget(close); root->addLayout(toolbar);
        scene_=new EditorScene(this); view_=new WorkflowGraphicsView(scene_,this); view_->installEventFilter(this); qApp->installEventFilter(this); workspace_.scene=scene_; workflow_workspace_init(&workspace_,scene_); resetUndo();
        managerUi_=create_workflow_manager_ui(workflow_workspace_manager(&workspace_),this,
            [this](const char *id){ if(workflow_workspace_select(&workspace_,id)) resetUndo(); },
            [this](const char *name){const bool ok=workflow_workspace_create(&workspace_,name); if(ok)resetUndo(); return ok;},
            [this](const char *name){const bool ok=workflow_workspace_duplicate(&workspace_,name); if(ok)resetUndo(); return ok;},
            [this]{return deleteWorkflow();},
            [this]{ workflow_workspace_sync_scene(&workspace_); undo_.capture(); });
        root->addWidget(managerUi_);
        auto *hint=new QLabel("Trigger nodes start workflow branches. Action nodes reference an existing Move / Swap / Value filter. Drag nodes, double-click to edit, use the mouse wheel to zoom and middle mouse to pan.",this); hint->setWordWrap(true); root->addWidget(hint); root->addWidget(view_,1);
        auto *status=new QHBoxLayout; status->addStretch(); status->addWidget(new QLabel("Zoom:",this)); zoomLabel_=new QLabel("100%",this); status->addWidget(zoomLabel_); root->addLayout(status); view_->setZoomLabel(zoomLabel_);
        connect(addButton_,&QPushButton::clicked,this,[this]{showAddNodeMenu();}); connect(edit,&QPushButton::clicked,this,[this]{editSelectedNode();}); connect(copyButton_,&QPushButton::clicked,this,[this]{copySelectedNodes();}); connect(pasteButton_,&QPushButton::clicked,this,[this]{pasteNodes();}); connect(duplicateButton_,&QPushButton::clicked,this,[this]{duplicateSelectedNode();}); connect(deleteButton_,&QPushButton::clicked,this,[this]{deleteSelectedNodes();});
        connect(zoomOut,&QPushButton::clicked,view_,&WorkflowGraphicsView::zoomOut); connect(zoomReset,&QPushButton::clicked,view_,&WorkflowGraphicsView::resetZoom); connect(zoomIn,&QPushButton::clicked,view_,&WorkflowGraphicsView::zoomIn); connect(fit,&QPushButton::clicked,view_,&WorkflowGraphicsView::fitAll); connect(close,&QPushButton::clicked,this,&QDialog::hide);
        connect(scene_,&QGraphicsScene::selectionChanged,this,[this]{updateButtonState();}); connect(scene_,&EditorScene::nodeDoubleClicked,this,[this](NodeItem *node){editNode(node);}); connect(scene_,&QGraphicsScene::changed,this,[this]{scheduleSync();});
        auto *undoShortcut=new QShortcut(QKeySequence::Undo,this); undoShortcut->setContext(Qt::WidgetWithChildrenShortcut); connect(undoShortcut,&QShortcut::activated,this,&EditorWindow::undoWorkflow);
        auto *deleteShortcut=new QShortcut(QKeySequence::Delete,this); deleteShortcut->setContext(Qt::WidgetWithChildrenShortcut); connect(deleteShortcut,&QShortcut::activated,this,&EditorWindow::deleteSelectedNodes); updateButtonState();
    }
    ~EditorWindow() override { if(qApp) qApp->removeEventFilter(this); }
public slots:
    void redoFromHotkey() { blog(LOG_INFO,"[Move Workflow] Ctrl+Y Qt slot reached."); redoWorkflow(); }
protected:
    bool eventFilter(QObject *watched,QEvent *event) override { return QDialog::eventFilter(watched,event); }
    void keyPressEvent(QKeyEvent *event) override { QDialog::keyPressEvent(event); }
private:
    void scheduleSync(){if(syncPending_||applyingUndoRedo_)return;syncPending_=true;const auto generation=syncGeneration_;QTimer::singleShot(0,this,[this,generation]{syncPending_=false;if(applyingUndoRedo_||generation!=syncGeneration_)return;workflow_workspace_sync_scene(&workspace_);undo_.capture();updateButtonState();});}
    void resetUndo(){if(applyingUndoRedo_){blog(LOG_INFO,"[Move Workflow] resetUndo suppressed during undo/redo replay.");return;}++syncGeneration_;syncPending_=false;undo_.reset(workflow_manager_selected(workflow_workspace_manager(&workspace_)),workflow_workspace_manager(&workspace_));}
    void syncLoadedSelection(){const auto *s=workflow_manager_selected_const(workflow_workspace_manager(&workspace_));if(s){std::strncpy(workspace_.loaded_workflow_id,s->id,WORKFLOW_MAX_NAME-1);workspace_.loaded_workflow_id[WORKFLOW_MAX_NAME-1]='\0';}else workspace_.loaded_workflow_id[0]='\0';}
    void refreshWorkflowManagerUi(){auto *combo=managerUi_?managerUi_->findChild<QComboBox *>():nullptr;if(!combo)return;const auto *selected=workflow_manager_selected_const(workflow_workspace_manager(&workspace_));const QString selectedId=selected?QString::fromUtf8(selected->id):QString();combo->blockSignals(true);combo->clear();auto *manager=workflow_workspace_manager(&workspace_);for(size_t i=0;i<manager->workflow_count;++i)combo->addItem(QString::fromUtf8(manager->workflows[i].name),QString::fromUtf8(manager->workflows[i].id));const int index=combo->findData(selectedId);combo->blockSignals(false);if(index>=0)combo->setCurrentIndex(index);else if(combo->count()>0)combo->setCurrentIndex(0);}
    void undoWorkflow(){++syncGeneration_;syncPending_=false;applyingUndoRedo_=true;const bool changed=undo_.undo();qDebug()<<"[Move Workflow] Ctrl+Z/Undo changed:"<<changed;if(changed){syncLoadedSelection();workflow_workspace_reload(&workspace_);workflow_persistence_sync(workflow_workspace_manager(&workspace_));refreshWorkflowManagerUi();updateButtonState();}applyingUndoRedo_=false;}
    void redoWorkflow(){++syncGeneration_;syncPending_=false;applyingUndoRedo_=true;blog(LOG_INFO,"[Move Workflow] Redo requested; canRedo: %d",undo_.canRedo());const bool changed=undo_.redo();blog(LOG_INFO,"[Move Workflow] Redo changed: %d",changed);if(changed){syncLoadedSelection();workflow_workspace_reload(&workspace_);workflow_persistence_sync(workflow_workspace_manager(&workspace_));refreshWorkflowManagerUi();updateButtonState();}applyingUndoRedo_=false;}
    bool deleteWorkflow(){workflow_workspace_sync_scene(&workspace_);auto *manager=workflow_workspace_manager(&workspace_);const auto *selected=workflow_manager_selected_const(manager);if(!selected)return false;undo_.prepareManagerCapture();const bool removed=workflow_manager_remove(manager,selected->id);if(!removed)return false;syncLoadedSelection();if(workspace_.loaded_workflow_id[0])workflow_workspace_reload(&workspace_);else{const QList<NodeItem *> nodes=scene_->nodes();for(NodeItem *node:nodes)scene_->deleteNode(node);}undo_.captureManager();workflow_persistence_sync(manager);updateButtonState();return true;}
    void showAddNodeMenu(){QMenu menu(this);QAction *trigger=menu.addAction("Add Trigger Node");QAction *action=menu.addAction("Add Action Node");QAction *chosen=menu.exec(addButton_->mapToGlobal(QPoint(0,addButton_->height())));if(!chosen)return;bool ok=false;const QString name=QInputDialog::getText(this,chosen==trigger?"Add Trigger Node":"Add Action Node","Node name:",QLineEdit::Normal,chosen==trigger?"New Trigger":"New Action",&ok);if(!ok||name.trimmed().isEmpty())return;NodeItem *node=scene_->addNode(chosen==trigger?WORKFLOW_NODE_TRIGGER:WORKFLOW_NODE_ACTION,name.trimmed());if(node){node->setSelected(true);view_->fitAll();}}
    void editNode(NodeItem *node){if(!node)return;if(edit_node_settings(node,scene_->nodes(),this)){node->refreshDisplay();scene_->refreshConnectionsFor(node);}}
    void editSelectedNode(){editNode(scene_->selectedNode());}
    void copySelectedNodes(){if(workflow_clipboard_copy(scene_))updateButtonState();}
    void pasteNodes(){if(workflow_clipboard_paste(scene_))updateButtonState();}
    void duplicateSelectedNode(){if(duplicate_selected_workflow_node(scene_))updateButtonState();}
    void deleteSelectedNodes(){const QList<QGraphicsItem *> selected=scene_->selectedItems();QList<NodeItem *> nodes;for(QGraphicsItem *item:selected)if(auto *node=dynamic_cast<NodeItem *>(item))nodes.append(node);if(nodes.isEmpty())return;workflow_workspace_sync_scene(&workspace_);for(NodeItem *node:nodes)scene_->deleteNode(node);workflow_workspace_sync_scene(&workspace_);undo_.capture();updateButtonState();}
    void updateButtonState(){const bool selected=scene_&&!scene_->selectedItems().isEmpty();deleteButton_->setEnabled(selected&&scene_->selectedNode());duplicateButton_->setEnabled(selected&&scene_->selectedNode());copyButton_->setEnabled(selected);pasteButton_->setEnabled(workflow_clipboard_has_data());}
    workflow_workspace_t workspace_{};WorkflowUndo undo_;QWidget *managerUi_=nullptr;EditorScene *scene_=nullptr;WorkflowGraphicsView *view_=nullptr;QLabel *zoomLabel_=nullptr;bool syncPending_=false;bool applyingUndoRedo_=false;unsigned long long syncGeneration_=0;QPushButton *addButton_=nullptr;QPushButton *copyButton_=nullptr;QPushButton *pasteButton_=nullptr;QPushButton *duplicateButton_=nullptr;QPushButton *deleteButton_=nullptr;
};
}
extern "C" void workflow_editor_redo_from_hotkey(void){EditorWindow *editor=window.data();if(!editor){blog(LOG_WARNING,"[Move Workflow] Ctrl+Y bridge fired but editor is not open.");return;}blog(LOG_INFO,"[Move Workflow] Ctrl+Y bridge reached editor; invoking Qt meta-object slot.");QMetaObject::invokeMethod(editor,"redoFromHotkey",Qt::QueuedConnection);}
void show_move_workflow_editor(QWidget *parent){if(!window){auto *mainWindow=parent?parent:static_cast<QWidget *>(obs_frontend_get_main_window());window=new EditorWindow(mainWindow);}window->show();window->raise();window->activateWindow();}

#include "workflow-editor-window.moc"
