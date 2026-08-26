#include "workflow-editor-properties.h"
#include "workflow-model.h"
#include "workflow-node.h"

#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <utility>

namespace {
constexpr uint64_t DEFAULT_START_DELAY_MS = 0;
constexpr uint64_t DEFAULT_DURATION_MS = 300;
constexpr uint64_t DEFAULT_END_DELAY_MS = 0;

QString listValues(size_t count, const char ids[][WORKFLOW_MAX_NAME])
{
    QStringList values;
    for (size_t i = 0; i < count; ++i)
        values << QString::fromUtf8(ids[i]);
    return values.isEmpty() ? QStringLiteral("None") : values.join(", ");
}

QString timingText(workflow_value_mode_t mode, uint64_t value, uint64_t defaultValue)
{
    if (mode == WORKFLOW_OVERRIDE)
        return QString("%1 ms").arg(static_cast<qulonglong>(value));
    return QString("%1 ms (default)").arg(static_cast<qulonglong>(defaultValue));
}

class EditorProperties final : public QWidget {
public:
    EditorProperties(QWidget *parent, std::function<void(NodeItem *)> editNode)
        : QWidget(parent), editNode_(std::move(editNode))
    {
        setObjectName("workflowEditorProperties");
        setMinimumWidth(280);
        setMaximumWidth(380);
        setStyleSheet("QWidget#workflowEditorProperties{background:#111820;border:1px solid #27313c;} "
                      "QLabel#propertiesHeading{color:#aab6c3;font-size:11px;font-weight:700;letter-spacing:1px;} "
                      "QFormLayout QLabel{color:#c5d0da;} QPushButton{background:#1b4f7c;color:#eef7ff;border:1px solid #2d78b4;border-radius:4px;padding:5px 10px;min-height:28px;} "
                      "QPushButton:hover{background:#245f91;} QPushButton:disabled{background:#18212a;color:#65727f;border-color:#29333d;}");
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(12, 10, 12, 12);
        root->setSpacing(8);
        auto *heading = new QLabel("NODE PROPERTIES", this);
        heading->setObjectName("propertiesHeading");
        root->addWidget(heading);
        auto *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        panel_ = new QWidget(scroll);
        form_ = new QFormLayout(panel_);
        form_->setContentsMargins(4, 4, 8, 8);
        form_->setVerticalSpacing(7);
        scroll->setWidget(panel_);
        root->addWidget(scroll, 1);
        edit_ = new QPushButton("Edit Node...", this);
        edit_->setEnabled(false);
        root->addWidget(edit_);
        connect(edit_, &QPushButton::clicked, this, [this] { if (node_ && editNode_) editNode_(node_); });
        clear();
    }

    void setNode(NodeItem *node)
    {
        node_ = node;
        edit_->setEnabled(node != nullptr);
        clear();
        if (!node)
            return;
        const auto *data = node->workflowNode();
        add("Name", data->name);
        add("ID", data->id);
        add("Type", workflow_node_type_name(data->type));

        if (data->type == WORKFLOW_NODE_TRIGGER) {
            add("Trigger Filters", QString::number(static_cast<qulonglong>(data->trigger_count)));
            for (size_t i = 0; i < data->trigger_count; ++i) {
                add(QString("Trigger %1 Source").arg(i + 1), data->triggers[i].source_uuid);
                add(QString("Trigger %1 Filter").arg(i + 1), data->triggers[i].filter_uuid);
            }
        } else if (data->type == WORKFLOW_NODE_ACTION) {
            add("Move Kind", workflow_move_kind_name(data->action.kind));
            add("Scene", data->action.scene_name);
            add("Source", data->action.source_name);
            add("Filter", data->action.filter_name);
            add("Filter ID", data->action.filter_id);
        }

        add("Start Delay", timingText(data->start_delay.mode, data->start_delay.delay_ms, DEFAULT_START_DELAY_MS));
        add("Duration", timingText(data->duration.mode, data->duration.duration_ms, DEFAULT_DURATION_MS));
        add("End Delay", timingText(data->end_delay.mode, data->end_delay.delay_ms, DEFAULT_END_DELAY_MS));
        add("Simultaneous Nodes", listValues(data->simultaneous_node_count, data->simultaneous_node_ids));
        add("Next Nodes", listValues(data->next_node_count, data->next_node_ids));
        add("Shortcut Nodes", listValues(data->shortcut_node_count, data->shortcut_node_ids));
    }

private:
    void add(const QString &name, const QString &value)
    {
        auto *label = new QLabel(value.isEmpty() ? QStringLiteral("None") : value, panel_);
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        form_->addRow(name, label);
    }
    void add(const QString &name, const char *value) { add(name, QString::fromUtf8(value ? value : "")); }
    void clear()
    {
        while (form_->rowCount() > 0)
            form_->removeRow(0);
        auto *label = new QLabel("No node selected", panel_);
        label->setStyleSheet("color:#7f8c99;");
        form_->addRow(label);
    }

    NodeItem *node_ = nullptr;
    std::function<void(NodeItem *)> editNode_;
    QWidget *panel_ = nullptr;
    QFormLayout *form_ = nullptr;
    QPushButton *edit_ = nullptr;
};
}

QWidget *create_workflow_editor_properties(QWidget *parent, std::function<void(NodeItem *)> edit_node)
{ return new EditorProperties(parent, std::move(edit_node)); }

void workflow_editor_properties_set_node(QWidget *properties, NodeItem *node)
{ if (auto *widget = dynamic_cast<EditorProperties *>(properties)) widget->setNode(node); }
