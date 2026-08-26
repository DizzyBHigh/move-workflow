#include "workflow-editor-sidebar.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {
class EditorSidebar final : public QWidget {
public:
    EditorSidebar(workflow_manager_t *manager, QWidget *parent, workflow_editor_sidebar_callbacks callbacks)
        : QWidget(parent), manager_(manager), callbacks_(std::move(callbacks)) {
        setObjectName("workflowEditorSidebar"); setMinimumWidth(230); setMaximumWidth(300);
        setStyleSheet("QWidget#workflowEditorSidebar{background:#111820;border:1px solid #27313c;} QLabel#sidebarHeading{color:#aab6c3;font-size:11px;font-weight:700;letter-spacing:1px;} QLineEdit{background:#10161d;color:#e6edf3;border:1px solid #2d3946;border-radius:4px;padding:5px 8px;min-height:24px;} QLineEdit:focus{border-color:#367fbe;} QListWidget{background:#10161d;color:#dbe4ec;border:1px solid #27333f;border-radius:4px;outline:0;} QListWidget::item{padding:6px 8px;border-radius:3px;} QListWidget::item:hover{background:#1b2834;} QListWidget::item:selected{background:#173f66;color:#ffffff;} QPushButton{background:#17212b;color:#dbe4ec;border:1px solid #2d3946;border-radius:4px;padding:3px 7px;min-height:25px;} QPushButton:hover{background:#20303d;border-color:#3d5266;} QPushButton:disabled{color:#687583;background:#141b22;} QCheckBox{color:#dbe4ec;spacing:7px;} QCheckBox::indicator{width:15px;height:15px;} QCheckBox::indicator:checked{background:#2b78b8;border:1px solid #4b98d4;border-radius:3px;} ");
        auto *root = new QVBoxLayout(this); root->setContentsMargins(10, 10, 10, 10); root->setSpacing(6);
        root->addWidget(title("WORKFLOWS")); workflowSearch_ = new QLineEdit(this); workflowSearch_->setPlaceholderText("Search workflows..."); root->addWidget(workflowSearch_);
        workflows_ = new QListWidget(this); workflows_->setObjectName("workflowList"); root->addWidget(workflows_, 1);
        auto *actions = new QHBoxLayout; actions->setSpacing(4); addWorkflow_ = button("+"); duplicate_ = button("Copy"); rename_ = button("Rename"); remove_ = button("Delete"); actions->addWidget(addWorkflow_); actions->addWidget(duplicate_); actions->addWidget(rename_); actions->addWidget(remove_); root->addLayout(actions);
        auto *files = new QHBoxLayout; files->setSpacing(4); import_ = button("Import"); export_ = button("Export"); files->addWidget(import_); files->addWidget(export_); root->addLayout(files);
        enabled_ = new QCheckBox("Workflow enabled", this); root->addWidget(enabled_);
        root->addSpacing(5); root->addWidget(title("NODE LIBRARY")); nodeSearch_ = new QLineEdit(this); nodeSearch_->setPlaceholderText("Search nodes..."); root->addWidget(nodeSearch_);
        nodes_ = new QListWidget(this); nodes_->setObjectName("nodePalette"); root->addWidget(nodes_, 1);
        addPaletteItem("⚡  Trigger", "trigger"); addPaletteItem("Move Action", "action"); addPaletteItem("Move Source", "action"); addPaletteItem("Move Source Swap", "action"); addPaletteItem("Move Value", "action"); addPaletteItem("Change Scene", "action");
        connect(workflowSearch_, &QLineEdit::textChanged, this, [this](const QString &text) { filter(workflows_, text); }); connect(nodeSearch_, &QLineEdit::textChanged, this, [this](const QString &text) { filter(nodes_, text); });
        connect(workflows_, &QListWidget::currentRowChanged, this, [this](int row) { selectWorkflow(row); }); connect(nodes_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) { if (callbacks_.add_node) callbacks_.add_node(item->data(Qt::UserRole).toByteArray().constData()); });
        connect(addWorkflow_, &QPushButton::clicked, this, [this] { createWorkflow(); }); connect(duplicate_, &QPushButton::clicked, this, [this] { duplicateWorkflow(); }); connect(rename_, &QPushButton::clicked, this, [this] { if (callbacks_.rename_workflow) callbacks_.rename_workflow(); }); connect(remove_, &QPushButton::clicked, this, [this] { if (callbacks_.delete_workflow) callbacks_.delete_workflow(); });
        connect(import_, &QPushButton::clicked, this, [this] { if (callbacks_.import_workflow) callbacks_.import_workflow(); }); connect(export_, &QPushButton::clicked, this, [this] { if (callbacks_.export_workflow) callbacks_.export_workflow(); }); connect(enabled_, &QCheckBox::toggled, this, [this](bool value) { if (callbacks_.set_workflow_enabled) callbacks_.set_workflow_enabled(value); }); refresh();
    }
    void refresh() {
        const auto *selected = workflow_manager_selected_const(manager_); const QString id = selected ? QString::fromUtf8(selected->id) : QString(); workflows_->blockSignals(true); workflows_->clear();
        for (size_t i = 0; i < manager_->workflow_count; ++i) { const auto &workflow = manager_->workflows[i]; auto *item = new QListWidgetItem(QString::fromUtf8(workflow.name), workflows_); item->setData(Qt::UserRole, QString::fromUtf8(workflow.id)); }
        for (int i = 0; i < workflows_->count(); ++i) if (workflows_->item(i)->data(Qt::UserRole).toString() == id) workflows_->setCurrentRow(i); workflows_->blockSignals(false); updateState();
    }
private:
    QLabel *title(const char *text) { auto *label = new QLabel(text, this); label->setObjectName("sidebarHeading"); return label; }
    QPushButton *button(const char *text) { return new QPushButton(text, this); }
    void addPaletteItem(const char *text, const char *kind) { auto *item = new QListWidgetItem(text, nodes_); item->setData(Qt::UserRole, kind); }
    void filter(QListWidget *list, const QString &text) { for (int i = 0; i < list->count(); ++i) list->item(i)->setHidden(!list->item(i)->text().contains(text, Qt::CaseInsensitive)); }
    void selectWorkflow(int row) { if (row < 0 || !callbacks_.select_workflow) return; const QByteArray id = workflows_->item(row)->data(Qt::UserRole).toString().toUtf8(); callbacks_.select_workflow(id.constData()); updateState(); }
    void createWorkflow() { bool ok = false; const QString name = QInputDialog::getText(this, "New Workflow", "Workflow name:", QLineEdit::Normal, "New Workflow", &ok); if (ok && !name.trimmed().isEmpty() && callbacks_.create_workflow) callbacks_.create_workflow(name.trimmed().toUtf8().constData()); refresh(); }
    void duplicateWorkflow() { const auto *selected = workflow_manager_selected_const(manager_); const QString base = selected ? QString::fromUtf8(selected->name) + " Copy" : "Workflow Copy"; bool ok = false; const QString name = QInputDialog::getText(this, "Duplicate Workflow", "Workflow name:", QLineEdit::Normal, base, &ok); if (ok && !name.trimmed().isEmpty() && callbacks_.duplicate_workflow) callbacks_.duplicate_workflow(name.trimmed().toUtf8().constData()); refresh(); }
    void updateState() { const auto *selected = workflow_manager_selected_const(manager_); const bool has = selected != nullptr; duplicate_->setEnabled(has); rename_->setEnabled(has); remove_->setEnabled(has); enabled_->setEnabled(has); enabled_->blockSignals(true); enabled_->setChecked(has && selected->enabled); enabled_->blockSignals(false); }
    workflow_manager_t *manager_; workflow_editor_sidebar_callbacks callbacks_; QLineEdit *workflowSearch_ = nullptr; QListWidget *workflows_ = nullptr; QCheckBox *enabled_ = nullptr; QListWidget *nodes_ = nullptr; QLineEdit *nodeSearch_ = nullptr;
    QPushButton *addWorkflow_ = nullptr; QPushButton *duplicate_ = nullptr; QPushButton *rename_ = nullptr; QPushButton *remove_ = nullptr; QPushButton *import_ = nullptr; QPushButton *export_ = nullptr;
};
}
QWidget *create_workflow_editor_sidebar(workflow_manager_t *manager, QWidget *parent, workflow_editor_sidebar_callbacks callbacks)
{return new EditorSidebar(manager, parent, std::move(callbacks));}
void workflow_editor_sidebar_refresh(QWidget *sidebar)
{if (auto *widget = dynamic_cast<EditorSidebar *>(sidebar)) widget->refresh();}
