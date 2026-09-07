#include "workflow-editor-properties.h"
#include "workflow-model.h"
#include "workflow-node.h"

#include <QFormLayout>
#include <QFrame>
#include <QGraphicsScene>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <utility>

namespace {
constexpr uint64_t DEFAULT_START_DELAY_MS = 0;
constexpr uint64_t DEFAULT_DURATION_MS = 300;
constexpr uint64_t DEFAULT_END_DELAY_MS = 0;

QString easingName(workflow_easing_t value)
{
    switch (value) {
    case WORKFLOW_EASE_IN: return "In";
    case WORKFLOW_EASE_OUT: return "Out";
    case WORKFLOW_EASE_IN_OUT: return "In / Out";
    default: return "None";
    }
}

QString easingFunctionName(workflow_easing_function_t value)
{
    switch (value) {
    case WORKFLOW_EASING_QUADRATIC: return "Quadratic";
    case WORKFLOW_EASING_CUBIC: return "Cubic";
    case WORKFLOW_EASING_QUARTIC: return "Quartic";
    case WORKFLOW_EASING_QUINTIC: return "Quintic";
    case WORKFLOW_EASING_SINE: return "Sine";
    case WORKFLOW_EASING_CIRCULAR: return "Circular";
    case WORKFLOW_EASING_EXPONENTIAL: return "Exponential";
    case WORKFLOW_EASING_ELASTIC: return "Elastic";
    case WORKFLOW_EASING_BOUNCE: return "Bounce";
    case WORKFLOW_EASING_BACK: return "Back";
    default: return "Unknown";
    }
}

QString nodeDisplay(QGraphicsScene *scene, const QString &id, bool showIds)
{
    if (showIds || !scene)
        return id;
    for (QGraphicsItem *item : scene->items()) {
        auto *node = dynamic_cast<NodeItem *>(item);
        if (node && node->id().compare(id, Qt::CaseInsensitive) == 0)
            return node->nodeName();
    }
    return id;
}

QString listValues(NodeItem *owner, size_t count, const char ids[][WORKFLOW_MAX_NAME], bool showIds)
{
    QStringList values;
    QGraphicsScene *scene = owner ? owner->scene() : nullptr;
    for (size_t i = 0; i < count; ++i)
        values << nodeDisplay(scene, QString::fromUtf8(ids[i]), showIds);
    return values.isEmpty() ? QStringLiteral("None") : values.join("\n");
}

QString shortcutListValues(NodeItem *owner, const workflow_node_t *data, bool showIds)
{
    QStringList values;
    QGraphicsScene *scene = owner ? owner->scene() : nullptr;
    for (size_t i = 0; i < data->shortcut_node_count; ++i) {
        const QString id = QString::fromUtf8(data->shortcut_node_ids[i]);
        QString value = nodeDisplay(scene, id, showIds);
        for (size_t j = 0; j < data->shortcut_binding_count; ++j) {
            const auto &binding = data->shortcut_bindings[j];
            if (QString::fromUtf8(binding.target_id).compare(id, Qt::CaseInsensitive) == 0) {
                const QString key = QString::fromUtf8(binding.key);
                if (!key.isEmpty()) value += QStringLiteral(" [") + key + QStringLiteral("]");
                break;
            }
        }
        values << value;
    }
    return values.isEmpty() ? QStringLiteral("None") : values.join("\n");
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
        setObjectName("workflowEditorProperties"); setMinimumWidth(280); setMaximumWidth(380);
        setStyleSheet("QWidget#workflowEditorProperties{background:#111820;border:1px solid #27313c;} "
                      "QLabel#propertiesHeading{color:#aab6c3;font-size:11px;font-weight:700;letter-spacing:1px;} "
                      "QFormLayout QLabel{color:#c5d0da;} QPushButton{background:#1b4f7c;color:#eef7ff;border:1px solid #2d78b4;border-radius:4px;padding:5px 10px;min-height:28px;} "
                      "QPushButton:hover{background:#245f91;} QPushButton:disabled{background:#18212a;color:#65727f;border-color:#29333d;}");
        auto *root = new QVBoxLayout(this); root->setContentsMargins(12, 10, 12, 12); root->setSpacing(8);
        auto *heading = new QLabel("NODE PROPERTIES", this); heading->setObjectName("propertiesHeading"); root->addWidget(heading);
        auto *scroll = new QScrollArea(this); scroll->setWidgetResizable(true); scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); scroll->setFrameShape(QFrame::NoFrame);
        panel_ = new QWidget(scroll); form_ = new QFormLayout(panel_); form_->setContentsMargins(4, 4, 8, 8); form_->setVerticalSpacing(7); scroll->setWidget(panel_); root->addWidget(scroll, 1);
        edit_ = new QPushButton("Edit Node...", this); edit_->setEnabled(false); root->addWidget(edit_);
        toggle_ = new QPushButton("Show IDs", this); toggle_->setCheckable(true); toggle_->setFixedWidth(90); toggle_->setMinimumHeight(24); toggle_->setToolTip("Switch relationship nodes between friendly names and IDs");
        connect(toggle_, &QPushButton::toggled, this, [this](bool checked) { showIds_ = checked; toggle_->setText(checked ? "Show Names" : "Show IDs"); if (node_) setNode(node_); });
        connect(edit_, &QPushButton::clicked, this, [this] { if (node_ && editNode_) editNode_(node_); }); clear();
    }

    void setNode(NodeItem *node)
    {
        node_ = node; edit_->setEnabled(node != nullptr); clear(); if (!node) return;
        const auto *data = node->workflowNode();
        add("Name", data->name); add("ID", data->id); add("Type", workflow_node_type_name(data->type));
        if (data->type == WORKFLOW_NODE_TRIGGER) {
            add("Trigger Filters", QString::number(static_cast<qulonglong>(data->trigger_count)));
            for (size_t i = 0; i < data->trigger_count; ++i) { add(QString("Trigger %1 Source").arg(i + 1), data->triggers[i].source_uuid); add(QString("Trigger %1 Filter").arg(i + 1), data->triggers[i].filter_uuid); }
        } else if (data->type == WORKFLOW_NODE_ACTION) {
            add("Move Kind", workflow_move_kind_name(data->action.kind)); add("Scene", data->action.scene_name); add("Source", data->action.source_name); add("Filter", data->action.filter_name); add("Filter ID", data->action.filter_id);
            add("Easing", easingName(data->easing.easing)); add("Easing Function", easingFunctionName(data->easing.function));
        }
        add("Start Delay", timingText(data->start_delay.mode, data->start_delay.delay_ms, DEFAULT_START_DELAY_MS)); add("Duration", timingText(data->duration.mode, data->duration.duration_ms, DEFAULT_DURATION_MS)); add("End Delay", timingText(data->end_delay.mode, data->end_delay.delay_ms, DEFAULT_END_DELAY_MS)); addConnections(data, node);
    }

private:
    void add(const QString &name, const QString &value) { auto *label = new QLabel(value.isEmpty() ? QStringLiteral("None") : value, panel_); label->setWordWrap(true); label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); label->setTextInteractionFlags(Qt::TextSelectableByMouse); form_->addRow(name, label); }
    void add(const QString &name, const char *value) { add(name, QString::fromUtf8(value ? value : "")); }
    void addConnections(const workflow_node_t *data, NodeItem *node) { form_->addRow(QStringLiteral("Connections"), toggle_); add("Next", listValues(node, data->next_node_count, data->next_node_ids, showIds_)); add("Simultaneous", listValues(node, data->simultaneous_node_count, data->simultaneous_node_ids, showIds_)); add("Shortcut", shortcutListValues(node, data, showIds_)); }
    void clear()
    {
        if (toggle_) form_->removeRow(toggle_);
        while (form_->rowCount() > 0) form_->removeRow(0);
        auto *label = new QLabel("No node selected", panel_); label->setStyleSheet("color:#7f8c99;"); form_->addRow(label);
    }

    NodeItem *node_ = nullptr; std::function<void(NodeItem *)> editNode_; QWidget *panel_ = nullptr; QFormLayout *form_ = nullptr; QPushButton *edit_ = nullptr; QPushButton *toggle_ = nullptr; bool showIds_ = false;
};
}

QWidget *create_workflow_editor_properties(QWidget *parent, std::function<void(NodeItem *)> edit_node)
{ return new EditorProperties(parent, std::move(edit_node)); }

void workflow_editor_properties_set_node(QWidget *properties, NodeItem *node)
{ if (auto *widget = dynamic_cast<EditorProperties *>(properties)) widget->setNode(node); }
