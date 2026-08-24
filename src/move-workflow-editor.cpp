#include "move-workflow-editor.h"
#include "workflow-editor-view.h"
#include "workflow-model.h"
#include "workflow-node-dialog.h"
#include "workflow-scene.h"

#include <obs-frontend-api.h>

#include <QDialog>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace {

class EditorWindow final : public QDialog {
public:
    explicit EditorWindow(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Move Workflow Editor");
        resize(1050, 700);

        auto *root = new QVBoxLayout(this);
        auto *toolbar = new QHBoxLayout;
        addButton_ = new QPushButton("+ Add Node", this);
        auto *edit = new QPushButton("Edit Node", this);
        deleteButton_ = new QPushButton("Delete Node", this);
        auto *zoomOut = new QPushButton("−", this);
        auto *zoomReset = new QPushButton("100%", this);
        auto *zoomIn = new QPushButton("+", this);
        auto *fit = new QPushButton("Fit All", this);
        auto *close = new QPushButton("Close", this);
        toolbar->addWidget(addButton_);
        toolbar->addWidget(edit);
        toolbar->addWidget(deleteButton_);
        toolbar->addStretch();
        toolbar->addWidget(zoomOut);
        toolbar->addWidget(zoomReset);
        toolbar->addWidget(zoomIn);
        toolbar->addWidget(fit);
        toolbar->addWidget(close);
        root->addLayout(toolbar);

        auto *hint = new QLabel(
            "Trigger nodes start workflow branches. Action nodes reference an existing Move / Swap / Value filter. "
            "Drag nodes, double-click to edit, use the mouse wheel to zoom and middle mouse to pan.", this);
        hint->setWordWrap(true);
        root->addWidget(hint);

        scene_ = new EditorScene(this);
        view_ = new WorkflowGraphicsView(scene_, this);
        root->addWidget(view_, 1);

        auto *status = new QHBoxLayout;
        status->addStretch();
        status->addWidget(new QLabel("Zoom:", this));
        zoomLabel_ = new QLabel("100%", this);
        status->addWidget(zoomLabel_);
        root->addLayout(status);
        view_->setZoomLabel(zoomLabel_);

        connect(addButton_, &QPushButton::clicked, this, [this] { showAddNodeMenu(); });
        connect(edit, &QPushButton::clicked, this, [this] { editSelectedNode(); });
        connect(deleteButton_, &QPushButton::clicked, this, [this] { deleteSelectedNode(); });
        connect(zoomOut, &QPushButton::clicked, view_, &WorkflowGraphicsView::zoomOut);
        connect(zoomReset, &QPushButton::clicked, view_, &WorkflowGraphicsView::resetZoom);
        connect(zoomIn, &QPushButton::clicked, view_, &WorkflowGraphicsView::zoomIn);
        connect(fit, &QPushButton::clicked, view_, &WorkflowGraphicsView::fitAll);
        connect(close, &QPushButton::clicked, this, &QDialog::hide);
        connect(scene_, &QGraphicsScene::selectionChanged, this, [this] { updateButtonState(); });
        connect(scene_, &EditorScene::nodeDoubleClicked, this, [this](NodeItem *node) { editNode(node); });
        updateButtonState();
    }

private:
    void showAddNodeMenu()
    {
        QMenu menu(this);
        QAction *trigger = menu.addAction("Add Trigger Node");
        QAction *action = menu.addAction("Add Action Node");
        QAction *chosen = menu.exec(addButton_->mapToGlobal(QPoint(0, addButton_->height())));
        if (!chosen)
            return;

        bool ok = false;
        const QString name = QInputDialog::getText(
            this,
            chosen == trigger ? "Add Trigger Node" : "Add Action Node",
            "Node name:",
            QLineEdit::Normal,
            chosen == trigger ? "New Trigger" : "New Action",
            &ok);
        if (!ok || name.trimmed().isEmpty())
            return;

        NodeItem *node = scene_->addNode(
            chosen == trigger ? WORKFLOW_NODE_TRIGGER : WORKFLOW_NODE_ACTION,
            name.trimmed());
        if (node) {
            node->setSelected(true);
            view_->fitAll();
        }
    }

    void editNode(NodeItem *node)
    {
        if (!node)
            return;
        if (edit_node_settings(node, scene_->nodes(), this)) {
            node->refreshDisplay();
            scene_->refreshConnectionsFor(node);
        }
    }

    void editSelectedNode()
    {
        editNode(scene_->selectedNode());
    }

    void deleteSelectedNode()
    {
        NodeItem *node = scene_->selectedNode();
        if (!node)
            return;
        const QString name = node->nodeName();
        if (QMessageBox::question(
                this,
                "Delete Node",
                QString("Delete '%1'?\n\nAny connections to this node will also be removed.").arg(name),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes)
            return;
        scene_->deleteNode(node);
        updateButtonState();
    }

    void updateButtonState()
    {
        const bool selected = scene_ && scene_->selectedNode();
        deleteButton_->setEnabled(selected);
    }

    EditorScene *scene_ = nullptr;
    WorkflowGraphicsView *view_ = nullptr;
    QLabel *zoomLabel_ = nullptr;
    QPushButton *addButton_ = nullptr;
    QPushButton *deleteButton_ = nullptr;
};

QPointer<EditorWindow> window;

void show_editor()
{
    if (!window) {
        auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        window = new EditorWindow(mainWindow);
    }
    window->show();
    window->raise();
    window->activateWindow();
}

void register_menu()
{
    QAction *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction("Move Workflow Editor"));
    if (!action)
        return;
    QObject::connect(action, &QAction::triggered, [] { show_editor(); });
}

struct AutoRegister {
    AutoRegister() { QTimer::singleShot(0, register_menu); }
};

AutoRegister auto_register;

} // namespace

#include "move-workflow-editor.moc"

void move_workflow_register_editor(void)
{
    QTimer::singleShot(0, register_menu);
}
