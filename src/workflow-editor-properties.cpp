#include "workflow-editor-properties.h"
#include "workflow-model.h"
#include "workflow-node.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {
class EditorProperties final : public QWidget {
public:
    EditorProperties(QWidget *parent, void (*editNode)(NodeItem *)) : QWidget(parent), editNode_(editNode) {
        setObjectName("workflowEditorProperties"); setMinimumWidth(250); setMaximumWidth(340);
        auto *root = new QVBoxLayout(this); root->setContentsMargins(12, 12, 12, 12); root->setSpacing(10);
        auto *heading = new QLabel("NODE PROPERTIES", this); heading->setObjectName("sidebarHeading"); root->addWidget(heading);
        group_ = new QGroupBox("No node selected", this); auto *form = new QFormLayout(group_);
        name_ = new QLabel("—", group_); type_ = new QLabel("—", group_); target_ = new QLabel("—", group_); timing_ = new QLabel("—", group_);
        form->addRow("Name", name_); form->addRow("Type", type_); form->addRow("Target", target_); form->addRow("Timing", timing_); root->addWidget(group_);
        edit_ = new QPushButton("Edit Node...", this); edit_->setEnabled(false); root->addWidget(edit_); root->addStretch();
        connect(edit_, &QPushButton::clicked, this, [this] { if (node_ && editNode_) editNode_(node_); });
    }
    void setNode(NodeItem *node) {
        node_ = node; edit_->setEnabled(node != nullptr);
        if (!node) { group_->setTitle("No node selected"); name_->setText("—"); type_->setText("—"); target_->setText("—"); timing_->setText("—"); return; }
        const auto *data = node->workflowNode(); group_->setTitle("Selected Node"); name_->setText(QString::fromUtf8(data->name)); type_->setText(QString::fromUtf8(workflow_node_type_name(data->type)));
        if (data->type == WORKFLOW_NODE_ACTION) target_->setText(QString::fromUtf8(data->action.filter_name));
        else target_->setText(QString::number(static_cast<qulonglong>(data->trigger_count)) + " trigger filter(s)");
        const auto duration = data->duration.mode == WORKFLOW_OVERRIDE ? QString::number(static_cast<qulonglong>(data->duration.duration_ms)) + " ms" : "Use existing";
        timing_->setText(duration);
    }
private:
    NodeItem *node_ = nullptr; void (*editNode_)(NodeItem *) = nullptr; QGroupBox *group_ = nullptr; QLabel *name_ = nullptr;
    QLabel *type_ = nullptr; QLabel *target_ = nullptr; QLabel *timing_ = nullptr; QPushButton *edit_ = nullptr;
};
}

QWidget *create_workflow_editor_properties(QWidget *parent, void (*edit_node)(NodeItem *))
{return new EditorProperties(parent, edit_node);}

void workflow_editor_properties_set_node(QWidget *properties, NodeItem *node)
{if (auto *widget = dynamic_cast<EditorProperties *>(properties)) widget->setNode(node);}
