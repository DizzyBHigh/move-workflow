#include "workflow-manager-ui.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace {

class WorkflowManagerWidget final : public QWidget {
public:
    WorkflowManagerWidget(workflow_manager_t *manager, QWidget *parent,
                          std::function<void(const char *)> selectionChanged,
                          std::function<bool(const char *)> createWorkflow,
                          std::function<bool(const char *)> duplicateWorkflow)
        : QWidget(parent), manager_(manager), selectionChanged_(std::move(selectionChanged)),
          createWorkflow_(std::move(createWorkflow)), duplicateWorkflow_(std::move(duplicateWorkflow))
    {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        combo_ = new QComboBox(this);
        add_ = new QPushButton("+ New", this);
        duplicate_ = new QPushButton("Duplicate", this);
        rename_ = new QPushButton("Rename", this);
        remove_ = new QPushButton("Delete", this);
        enabled_ = new QCheckBox("Enabled", this);
        layout->addWidget(combo_, 1);
        layout->addWidget(add_);
        layout->addWidget(duplicate_);
        layout->addWidget(rename_);
        layout->addWidget(remove_);
        layout->addWidget(enabled_);
        refresh();
        connect(combo_, &QComboBox::currentIndexChanged, this, [this](int index) {
            if (index >= 0) {
                const QByteArray id = combo_->itemData(index).toByteArray();
                if (workflow_manager_set_selected(manager_, id.constData()) && selectionChanged_)
                    selectionChanged_(id.constData());
            }
            refresh();
        });
        connect(add_, &QPushButton::clicked, this, [this] { addWorkflow(); });
        connect(duplicate_, &QPushButton::clicked, this, [this] { duplicateWorkflow(); });
        connect(rename_, &QPushButton::clicked, this, [this] { renameWorkflow(); });
        connect(remove_, &QPushButton::clicked, this, [this] { removeWorkflow(); });
        connect(enabled_, &QCheckBox::toggled, this, [this](bool enabled) {
            const auto *selected = workflow_manager_selected_const(manager_);
            if (selected)
                workflow_manager_set_enabled(manager_, selected->id, enabled);
        });
    }

    void refresh()
    {
        const auto *selected = workflow_manager_selected_const(manager_);
        const QString id = selected ? QString::fromUtf8(selected->id) : QString();
        combo_->blockSignals(true);
        combo_->clear();
        for (size_t i = 0; i < manager_->workflow_count; ++i)
            combo_->addItem(QString::fromUtf8(manager_->workflows[i].name),
                            QString::fromUtf8(manager_->workflows[i].id));
        const int index = combo_->findData(id);
        combo_->setCurrentIndex(index >= 0 ? index : 0);
        combo_->blockSignals(false);
        selected = workflow_manager_selected_const(manager_);
        const bool has = selected != nullptr;
        duplicate_->setEnabled(has);
        rename_->setEnabled(has);
        remove_->setEnabled(has);
        enabled_->setEnabled(has);
        enabled_->blockSignals(true);
        enabled_->setChecked(has && selected->enabled);
        enabled_->blockSignals(false);
    }

private:
    void addWorkflow()
    {
        bool ok = false;
        const QString name = QInputDialog::getText(this, "New Workflow", "Workflow name:",
                                                   QLineEdit::Normal, "New Workflow", &ok);
        if (!ok || name.trimmed().isEmpty())
            return;
        if (createWorkflow_ && createWorkflow_(name.trimmed().toUtf8().constData()))
            refresh();
    }

    void duplicateWorkflow()
    {
        bool ok = false;
        const auto *selected = workflow_manager_selected_const(manager_);
        const QString defaultName = selected
            ? QString::fromUtf8(selected->name) + " Copy" : "Workflow Copy";
        const QString name = QInputDialog::getText(this, "Duplicate Workflow", "Workflow name:",
                                                   QLineEdit::Normal, defaultName, &ok);
        if (!ok || name.trimmed().isEmpty())
            return;
        if (duplicateWorkflow_ && duplicateWorkflow_(name.trimmed().toUtf8().constData()))
            refresh();
    }

    void renameWorkflow()
    {
        auto *selected = workflow_manager_selected(manager_);
        if (!selected)
            return;
        bool ok = false;
        const QString name = QInputDialog::getText(this, "Rename Workflow", "Workflow name:",
                                                   QLineEdit::Normal, QString::fromUtf8(selected->name), &ok);
        if (ok && !name.trimmed().isEmpty()) {
            snprintf(selected->name, sizeof(selected->name), "%s", name.trimmed().toUtf8().constData());
            refresh();
        }
    }

    void removeWorkflow()
    {
        const auto *selected = workflow_manager_selected_const(manager_);
        if (selected)
            workflow_manager_remove(manager_, selected->id);
        refresh();
        const auto *next = workflow_manager_selected_const(manager_);
        if (next && selectionChanged_)
            selectionChanged_(next->id);
    }

    workflow_manager_t *manager_;
    std::function<void(const char *)> selectionChanged_;
    std::function<bool(const char *)> createWorkflow_;
    std::function<bool(const char *)> duplicateWorkflow_;
    QComboBox *combo_;
    QPushButton *add_;
    QPushButton *duplicate_;
    QPushButton *rename_;
    QPushButton *remove_;
    QCheckBox *enabled_;
};

} // namespace

QWidget *create_workflow_manager_ui(workflow_manager_t *manager, QWidget *parent,
                                    std::function<void(const char *)> selectionChanged,
                                    std::function<bool(const char *)> createWorkflow,
                                    std::function<bool(const char *)> duplicateWorkflow)
{
    return new WorkflowManagerWidget(manager, parent, std::move(selectionChanged),
                                     std::move(createWorkflow), std::move(duplicateWorkflow));
}
