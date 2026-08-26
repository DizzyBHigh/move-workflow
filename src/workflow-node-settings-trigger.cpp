#include "workflow-node-settings.h"
#include "workflow-action-list.h"
#include "workflow-node-settings-common.h"
#include "workflow-persistence.h"
#include <obs.h>
#include <cstring>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace {
static bool add_source(void *data, obs_source_t *source)
{
    auto *combo = static_cast<QComboBox *>(data);
    if (combo && source && combo->findData(obs_source_get_uuid(source)) < 0)
        combo->addItem(QString::fromUtf8(obs_source_get_name(source)), QString::fromUtf8(obs_source_get_uuid(source)));
    return true;
}
static void add_filter(obs_source_t *, obs_source_t *filter, void *data)
{
    auto *combo = static_cast<QComboBox *>(data);
    if (!combo || !workflow_trigger_filter_is_instance(filter)) return;
    char workflow[WORKFLOW_MAX_NAME]{}, trigger[WORKFLOW_MAX_NAME]{};
    workflow_trigger_filter_get_target(filter, workflow, trigger);
    combo->addItem(trigger[0] ? QString::fromUtf8(trigger) : QString("Unassigned"), QString::fromUtf8(obs_source_get_uuid(filter)));
}
static QString selected(QComboBox *combo)
{
    return combo->currentData().toString().isEmpty() ? combo->currentText().trimmed() : combo->currentData().toString();
}
}
void NodeSettingsDialog::buildTriggerEditor(QVBoxLayout *layout, QVBoxLayout *contentLayout)
{
    triggerBox_ = new QGroupBox("Triggered by", this); triggerRowsLayout_ = new QVBoxLayout(triggerBox_); layout->addWidget(triggerBox_);
    rebuildTriggerRows(); auto *add = new QPushButton("+ Add Trigger", triggerBox_); triggerRowsLayout_->addWidget(add); connect(add, &QPushButton::clicked, this, [this] { addTriggerRow(); });
    startActions_ = new WorkflowActionList("Start Actions", "These actions start when this Trigger fires. Multiple actions run in parallel.", node_, nodes_, node_->workflowNode()->simultaneous_node_ids, node_->workflowNode()->simultaneous_node_count, this); contentLayout->addWidget(startActions_);
    auto *hint = new QLabel("Any referenced Workflow Trigger Filter can start this Trigger Node.", this); hint->setWordWrap(true); hint->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); contentLayout->addWidget(hint);
}
void NodeSettingsDialog::rebuildTriggerRows()
{
    while (triggerRowsLayout_ && triggerRowsLayout_->count()) { auto *item = triggerRowsLayout_->takeAt(0); if (item->widget()) item->widget()->deleteLater(); delete item; }
    triggerRows_.clear(); const auto &node = *node_->workflowNode(); for (size_t i = 0; i < node.trigger_count; ++i) addTriggerRow({QString::fromUtf8(node.triggers[i].source_uuid), QString::fromUtf8(node.triggers[i].filter_uuid)}); if (triggerRows_.isEmpty()) addTriggerRow();
}
void NodeSettingsDialog::addTriggerRow(const TriggerSelection &selection)
{
    auto *row = new QHBoxLayout; auto *source = new QComboBox(triggerBox_); auto *trigger = new QComboBox(triggerBox_); auto *remove = new QPushButton("−", triggerBox_); remove->setFixedWidth(28); settings_searchable(source); settings_searchable(trigger);
    populateTriggerSources(source, selection.sourceUuid); populateTriggerFilters(trigger, selected(source), selection.filterUuid); row->addWidget(source, 1); row->addWidget(trigger, 1); row->addWidget(remove); triggerRowsLayout_->insertLayout(triggerRowsLayout_->count(), row); triggerRows_.push_back({source, trigger, remove});
    connect(source, &QComboBox::currentIndexChanged, this, [this, source, trigger] { populateTriggerFilters(trigger, selected(source)); });
    connect(remove, &QPushButton::clicked, this, [this, remove] { for (int i = 0; i < triggerRows_.size(); ++i) if (triggerRows_[i].remove == remove) { auto *item = triggerRowsLayout_->takeAt(i); delete item; triggerRows_.removeAt(i); break; } });
}
void NodeSettingsDialog::populateTriggerSources(QComboBox *combo, const QString &wanted)
{
    if (!combo) return; combo->blockSignals(true); combo->clear(); obs_enum_scenes(add_source, combo); obs_enum_sources(add_source, combo); const int index = combo->findData(wanted); if (index >= 0) combo->setCurrentIndex(index); else if (combo->count()) combo->setCurrentIndex(0); combo->blockSignals(false);
}
void NodeSettingsDialog::populateTriggerFilters(QComboBox *combo, const QString &sourceUuid, const QString &wanted)
{
    if (!combo) return; combo->blockSignals(true); combo->clear(); obs_source_t *source = obs_get_source_by_uuid(sourceUuid.toUtf8().constData()); if (source) { obs_source_enum_filters(source, add_filter, combo); obs_source_release(source); } const int index = combo->findData(wanted); if (index >= 0) combo->setCurrentIndex(index); combo->blockSignals(false);
}
bool NodeSettingsDialog::applyTrigger()
{
    auto *node = node_->workflowNode(); auto *manager = workflow_persistence_manager(); auto *workflow = manager ? workflow_manager_selected(manager) : nullptr; if (!workflow) return false;
    for (size_t i = 0; i < node->trigger_count; ++i) { auto *filter = workflow_trigger_filter_find(node->triggers[i].source_uuid, node->triggers[i].filter_uuid); if (!filter) continue; char oldWorkflow[WORKFLOW_MAX_NAME]{}, oldTrigger[WORKFLOW_MAX_NAME]{}; workflow_trigger_filter_get_target(filter, oldWorkflow, oldTrigger); if (!std::strcmp(oldWorkflow, workflow->id) && !std::strcmp(oldTrigger, node->id)) workflow_trigger_filter_set_target(filter, "", ""); obs_source_release(filter); }
    node->trigger_count = 0;
    for (const auto &row : triggerRows_) { const QString sourceUuid = selected(row.source), filterUuid = selected(row.trigger); if (sourceUuid.isEmpty() || filterUuid.isEmpty() || node->trigger_count >= WORKFLOW_MAX_TRIGGERS) continue; auto &ref = node->triggers[node->trigger_count++]; settings_copy_text(ref.source_uuid, WORKFLOW_MAX_NAME, sourceUuid); settings_copy_text(ref.filter_uuid, WORKFLOW_MAX_NAME, filterUuid); auto *filter = workflow_trigger_filter_find(ref.source_uuid, ref.filter_uuid); if (filter) { workflow_trigger_filter_set_target(filter, workflow->id, node->id); obs_source_release(filter); } }
    startActions_->apply(node->simultaneous_node_count, node->simultaneous_node_ids); node->simultaneous_actions_mode = WORKFLOW_OVERRIDE; return true;
}
