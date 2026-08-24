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
#include <obs-frontend-api.h>
#include <QDialog>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>
#include <cstring>

namespace {
class EditorWindow final : public QDialog {
public:
    explicit EditorWindow(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Move Workflow Editor"); resize(1050, 700);
        auto *root = new QVBoxLayout(this); auto *toolbar = new QHBoxLayout;
        addButton_=new QPushButton("+ Add Node",this); auto *edit=new QPushButton("Edit Node",this);
        copyButton_=new QPushButton("Copy",this); pasteButton_=new QPushButton("Paste",this);
        duplicateButton_=new QPushButton("Duplicate Node",this); deleteButton_=new QPushButton("Delete Node",this);
        auto *zoomOut=new QPushButton("−",this); auto *zoomReset=new QPushButton("100%",this); auto *zoomIn=new QPushButton("+",this); auto *fit=new QPushButton("Fit All",this); auto *close=new QPushButton("Close",this);
        toolbar->addWidget(addButton_); toolbar->addWidget(edit); toolbar->addWidget(copyButton_); toolbar->addWidget(pasteButton_); toolbar->addWidget(duplicateButton_); toolbar->addWidget(deleteButton_); toolbar->addStretch(); toolbar->addWidget(zoomOut); toolbar->addWidget(zoomReset); toolbar->addWidget(zoomIn); toolbar->addWidget(fit); toolbar->addWidget(close); root->addLayout(toolbar);
        scene_=new EditorScene(this); view_=new WorkflowGraphicsView(scene_,this); workspace_.scene=scene_; workflow_workspace_init(&workspace_,scene_); resetUndo();
        root->addWidget(create_workflow_manager_ui(workflow_workspace_manager(&workspace_),this,
            [this](const char *id){ if(workflow_workspace_select(&workspace_,id)) resetUndo(); },
            [this](const char *name){const bool ok=workflow_workspace_create(&workspace_,name); if(ok)resetUndo(); return ok;},
            [this](const char *name){const bool ok=workflow_workspace_duplicate(&workspace_,name); if(ok)resetUndo(); return ok;},
            [this]{return deleteWorkflow();},
            [this]{ workflow_workspace_sync_scene(&workspace_); undo_.capture(); }));
        auto *hint=new QLabel("Trigger nodes start workflow branches. Action nodes reference an existing Move / Swap / Value filter. Drag nodes, double-click to edit, use the mouse wheel to zoom and middle mouse to pan.",this); hint->setWordWrap(true); root->addWidget(hint); root->addWidget(view_,1);
        auto *status=new QHBoxLayout; status->addStretch(); status->addWidget(new QLabel("Zoom:",this)); zoomLabel_=new QLabel("100%",this); status->addWidget(zoomLabel_); root->addLayout(status); view_->setZoomLabel(zoomLabel_);
        connect(addButton_,&QPushButton::clicked,this,[this]{showAddNodeMenu();}); connect(edit,&QPushButton::clicked,this,[this]{editSelectedNode();}); connect(copyButton_,&QPushButton::clicked,this,[this]{copySelectedNodes();}); connect(pasteButton_,&QPushButton::clicked,this,[this]{pasteNodes();}); connect(duplicateButton_,&QPushButton::clicked,this,[this]{duplicateSelectedNode();}); connect(deleteButton_,&QPushButton::clicked,this,[this]{deleteSelectedNode();});
        connect(zoomOut,&QPushButton::clicked,view_,&WorkflowGraphicsView::zoomOut); connect(zoomReset,&QPushButton::clicked,view_,&WorkflowGraphicsView::resetZoom); connect(zoomIn,&QPushButton::clicked,view_,&WorkflowGraphicsView::zoomIn); connect(fit,&QPushButton::clicked,view_,&WorkflowGraphicsView::fitAll); connect(close,&QPushButton::clicked,this,&QDialog::hide);
        connect(scene_,&QGraphicsScene::selectionChanged,this,[this]{updateButtonState();}); connect(scene_,&EditorScene::nodeDoubleClicked,this,[this](NodeItem *node){editNode(node);}); connect(scene_,&QGraphicsScene::changed,this,[this]{scheduleSync();});
        new QShortcut(QKeySequence::Undo,this,[this]{undoWorkflow();}); new QShortcut(QKeySequence::Redo,this,[this]{redoWorkflow();}); updateButtonState();
    }
private:
    void scheduleSync(){if(syncPending_)return;syncPending_=true;QTimer::singleShot(0,this,[this]{syncPending_=false;workflow_workspace_sync_scene(&workspace_);undo_.capture();updateButtonState();});}
    void resetUndo(){undo_.reset(workflow_manager_selected(workflow_workspace_manager(&workspace_)),workflow_workspace_manager(&workspace_));}
    void syncLoadedSelection(){const auto *s=workflow_manager_selected_const(workflow_workspace_manager(&workspace_));if(s){std::strncpy(workspace_.loaded_workflow_id,s->id,WORKFLOW_MAX_NAME-1);workspace_.loaded_workflow_id[WORKFLOW_MAX_NAME-1]='\0';}else workspace_.loaded_workflow_id[0]='\0';}
    void undoWorkflow(){if(!undo_.undo())return;syncLoadedSelection();workflow_workspace_reload(&workspace_);workflow_persistence_sync(workflow_workspace_manager(&workspace_));updateButtonState();}
    void redoWorkflow(){if(!undo_.redo())return;syncLoadedSelection();workflow_workspace_reload(&workspace_);workflow_persistence_sync(workflow_workspace_manager(&workspace_));updateButtonState();}
    bool deleteWorkflow(){workflow_workspace_sync_scene(&workspace_);auto *manager=workflow_workspace_manager(&workspace_);const auto *selected=workflow_manager_selected_const(manager);if(!selected)return false;undo_.prepareManagerCapture();const bool removed=workflow_manager_remove(manager,selected->id);if(!removed)return false;syncLoadedSelection();if(workspace_.loaded_workflow_id[0])workflow_workspace_reload(&workspace_);else{const QList<NodeItem *> nodes=scene_->nodes();for(NodeItem *node:nodes)scene_->deleteNode(node);}undo_.captureManager();workflow_persistence_sync(manager);updateButtonState();return true;}
    void showAddNodeMenu(){QMenu menu(this);QAction *trigger=menu.addAction("Add Trigger Node");QAction *action=menu.addAction("Add Action Node");QAction *chosen=menu.exec(addButton_->mapToGlobal(QPoint(0,addButton_->height())));if(!chosen)return;bool ok=false;const QString name=QInputDialog::getText(this,chosen==trigger?"Add Trigger Node":"Add Action Node","Node name:",QLineEdit::Normal,chosen==trigger?"New Trigger":"New Action",&ok);if(!ok||name.trimmed().isEmpty())return;NodeItem *node=scene_->addNode(chosen==trigger?WORKFLOW_NODE_TRIGGER:WORKFLOW_NODE_ACTION,name.trimmed());if(node){node->setSelected(true);view_->fitAll();}}
    void editNode(NodeItem *node){if(!node)return;if(edit_node_settings(node,scene_->nodes(),this)){node->refreshDisplay();scene_->refreshConnectionsFor(node);}}
    void editSelectedNode(){editNode(scene_->selectedNode());}
    void copySelectedNodes(){if(workflow_clipboard_copy(scene_))updateButtonState();}
    void pasteNodes(){if(workflow_clipboard_paste(scene_))updateButtonState();}
    void duplicateSelectedNode(){if(duplicate_selected_workflow_node(scene_))updateButtonState();}
    void deleteSelectedNode(){NodeItem *node=scene_->selectedNode();if(!node)return;const QString name=node->nodeName();if(QMessageBox::question(this,"Delete Node",QString("Delete '%1'?\n\nAny connections to this node will also be removed.").arg(name),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;scene_->deleteNode(node);updateButtonState();}
    void updateButtonState(){const bool selected=scene_&&!scene_->selectedItems().isEmpty();deleteButton_->setEnabled(selected&&scene_->selectedNode());duplicateButton_->setEnabled(selected&&scene_->selectedNode());copyButton_->setEnabled(selected);pasteButton_->setEnabled(workflow_clipboard_has_data());}
    workflow_workspace_t workspace_{};WorkflowUndo undo_;EditorScene *scene_=nullptr;WorkflowGraphicsView *view_=nullptr;QLabel *zoomLabel_=nullptr;bool syncPending_=false;
    QPushButton *addButton_=nullptr;QPushButton *copyButton_=nullptr;QPushButton *pasteButton_=nullptr;QPushButton *duplicateButton_=nullptr;QPushButton *deleteButton_=nullptr;
};
QPointer<EditorWindow> window;
}
void show_move_workflow_editor(QWidget *parent){if(!window){auto *mainWindow=parent?parent:static_cast<QWidget *>(obs_frontend_get_main_window());window=new EditorWindow(mainWindow);}window->show();window->raise();window->activateWindow();}
