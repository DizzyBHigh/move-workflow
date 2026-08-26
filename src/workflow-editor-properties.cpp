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
    EditorProperties(QWidget *parent, std::function<void(NodeItem *)> editNode) : QWidget(parent), editNode_(std::move(editNode)) {
        setObjectName("workflowEditorProperties"); setMinimumWidth(250); setMaximumWidth(340);
        setStyleSheet("QWidget#workflowEditorProperties{background:#111820;border:1px solid #27313c;} QLabel#propertiesHeading{color:#aab6c3;font-size:11px;font-weight:700;letter-spacing:1px;} QGroupBox{background:#141d26;color:#e6edf3;border:1px solid #293643;border-radius:5px;margin-top:10px;padding:10px;} QGroupBox::title{color:#dce6ef;subcontrol-origin:margin;left:10px;padding:0 4px;} QFormLayout QLabel{color:#9eacb9;} QPushButton{background:#1b4f7c;color:#eef7ff;border:1px solid #2d78b4;border-radius:4px;padding:5px 10px;min-height:28px;} QPushButton:hover{background:#245f91;} QPushButton:disabled{background:#18212a;color:#65727f;border-color:#29333d;} ");
        auto *root = new QVBoxLayout(this); root->setContentsMargins(12, 10, 12, 12); root->setSpacing(8);
        auto *heading = new QLabel("NODE PROPERTIES", this); heading->setObjectName("propertiesHeading"); root->addWidget(heading);
        group_ = new QGroupBox("No node selected", this); auto *form = new QFormLayout(group_); form->setContentsMargins(10, 12, 10, 10); form->setVerticalSpacing(8);
        name_ = new QLabel("—", group_); type_ = new QLabel("—", group_); target_ = new QLabel("—", group_); timing_ = new QLabel("—", group_);
        for (auto *label : {name_, type_, target_, timing_}) { label->setWordWrap(true); label->setTextInteractionFlags(Qt::TextSelectableByMouse); }
        form->addRow("Name", name_); form->addRow("Type", type_); form->addRow("Target", target_); form->addRow("Timing", timing_); root->addWidget(group_);
        edit_ = new QPushButton("Edit Node...", this); edit_->setEnabled(false); root->addWidget(edit_); root->addStretch();
        connect(edit_, &QPushButton::clicked, this, [this] { if (node_ && editNode_) editNode_(node_); });
    }
    void setNode(NodeItem *node) {
        node_ = node; edit_->setEnabled(node != nullptr);
        if (!node) { group_->setTitle("No node selected"); name_->setText("—"); type_->setText("—"); target_->setText("—"); timing_->setText("—"); return; }
        const auto *data = node->workflowNode(); group_->setTitle("Selected Node"); name_->setText(QString::fromUtf8(data->name)); type_->setText(QString::fromUtf8(workflow_node_type_name(data->type)));
        if (data->type == WORKFLOW_NODE_ACTION) target_->setText(QString::fromUtf8(data->action.filter_name)); else target_->setText(QString::number(static_cast<qulonglong>(data->trigger_count)) + " trigger filter(s)");
        const auto duration = data->duration.mode == WORKFLOW_OVERRIDE ? QString::number(static_cast<qulonglong>(data->duration.duration_ms)) + " ms" : "Use existing"; timing_->setText(duration);
    }
private:
    NodeItem *node_ = nullptr; std::function<void(NodeItem *)> editNode_; QGroupBox *group_ = nullptr; QLabel *name_ = nullptr; QLabel *type_ = nullptr; QLabel *target_ = nullptr; QLabel *timing_ = nullptr; QPushButton *edit_ = nullptr;
};
}
QWidget *create_workflow_editor_properties(QWidget *parent, std::function<void(NodeItem *)> edit_node)
{return new EditorProperties(parent, std::move(edit_node));}
void workflow_editor_properties_set_node(QWidget *properties, NodeItem *node)
{if (auto *widget = dynamic_cast<EditorProperties *>(properties)) widget->setNode(node);}
