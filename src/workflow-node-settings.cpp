#include "workflow-node-settings.h"

#include "workflow-action-list.h"
#include "workflow-node-settings-common.h"

#include <obs.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

NodeSettingsDialog::NodeSettingsDialog(NodeItem *node, const QList<NodeItem *> &nodes,
                                       QWidget *parent)
    : QDialog(parent), node_(node), nodes_(nodes)
{
    setWindowTitle(QString("Node Settings - %1").arg(node ? node->nodeName() : "Node"));
    resize(560, 800);
    setMinimumSize(500, 400);

    auto *root = new QVBoxLayout(this);
    auto *area = new QScrollArea(this);
    area->setWidgetResizable(true);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setSpacing(8);

    const bool trigger = node && node->workflowNode()->type == WORKFLOW_NODE_TRIGGER;
    auto *box = new QGroupBox(trigger ? "Trigger Node" : "Node", content);
    auto *boxLayout = new QVBoxLayout(box);
    name_ = new QLineEdit(node ? node->nodeName() : QString(), box);
    boxLayout->addWidget(new QLabel("Name", box));
    boxLayout->addWidget(name_);
    layout->addWidget(box);
    if (trigger)
        buildTriggerEditor(boxLayout, layout);
    else
        buildActionEditor(content, layout);

    layout->addStretch(1);
    area->setWidget(content);
    root->addWidget(area, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (apply())
            accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

bool NodeSettingsDialog::apply()
{
    if (!node_)
        return false;
    const QString name = name_->text().trimmed();
    if (name.isEmpty())
        return false;

    workflow_node_t *wf = node_->workflowNode();
    settings_copy_text(wf->name, WORKFLOW_MAX_NAME, name);
    if (wf->type == WORKFLOW_NODE_TRIGGER)
        return applyTrigger();

    const QString parentName = source_->currentData().toString().isEmpty()
                                    ? source_->currentText().trimmed()
                                    : source_->currentData().toString();
    const QString filterName = filter_->currentData().toString();
    if (!parentName.isEmpty() && !filterName.isEmpty()) {
        obs_source_t *parent = obs_get_source_by_name(parentName.toUtf8().constData());
        if (parent) {
            obs_source_t *filter = obs_source_get_filter_by_name(
                parent, filterName.toUtf8().constData());
            if (filter) {
                const char *filterId = obs_source_get_id(filter);
                if (settings_supported_filter(filterId)) {
                    settings_copy_text(wf->action.scene_name, WORKFLOW_MAX_NAME, parentName);
                    wf->action.source_name[0] = '\0';
                    settings_copy_text(wf->action.filter_name, WORKFLOW_MAX_NAME, filterName);
                    settings_copy_text(wf->action.filter_id, WORKFLOW_MAX_NAME,
                                       QString::fromUtf8(filterId));
                    wf->action.kind = settings_kind(filterId);
                }
                obs_source_release(filter);
            }
            obs_source_release(parent);
        }
    }

    wf->start_delay.mode = startDelayDefault_->isChecked() ? WORKFLOW_USE_EXISTING : WORKFLOW_OVERRIDE;
    wf->start_delay.delay_ms = (uint64_t)startDelayMs_->value();
    wf->duration.mode = durationDefault_->isChecked() ? WORKFLOW_USE_EXISTING : WORKFLOW_OVERRIDE;
    wf->duration.duration_ms = (uint64_t)durationMs_->value();
    wf->end_delay.mode = endDelayDefault_->isChecked() ? WORKFLOW_USE_EXISTING : WORKFLOW_OVERRIDE;
    wf->end_delay.delay_ms = (uint64_t)endDelayMs_->value();
    simultaneous_->apply(wf->simultaneous_node_count, wf->simultaneous_node_ids);
    nextActions_->apply(wf->next_node_count, wf->next_node_ids);
    shortcutActions_->apply(wf->shortcut_node_count, wf->shortcut_node_ids);
    wf->simultaneous_actions_mode = WORKFLOW_OVERRIDE;
    wf->next_actions_mode = WORKFLOW_OVERRIDE;
    return true;
}

bool edit_node_settings(NodeItem *node, const QList<NodeItem *> &nodes, QWidget *parent)
{
    NodeSettingsDialog dialog(node, nodes, parent);
    return dialog.exec() == QDialog::Accepted;
}