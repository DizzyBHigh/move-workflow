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
    void redoFromHotkey() { redoWorkflow(); }
protected:
    bool eventFilter(QObject *watched,QEvent *event) override { return QDialog::eventFilter(watched,event); }
    void keyPressEvent(QKeyEvent *event) override { QDialog::keyPressEvent(event); }
};
}
#include "workflow-editor-window.moc"
