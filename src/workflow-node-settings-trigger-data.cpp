#include "workflow-node-settings.h"

#include "workflow-action-list.h"
#include "workflow-node-settings-common.h"

#include <obs.h>

#include <QComboBox>
#include <QSpinBox>

static bool add_trigger_source(void *data, obs_source_t *source)
{
    auto *combo = static_cast<QComboBox *>(data);
    if (!combo || !source)
        return true;
    const QString name = QString::fromUtf8(obs_source_get_name(source));
    if (combo->findData(name) < 0)
        combo->addItem(name, name);
    return true;
}

void NodeSettingsDialog::populateTriggerSources(const QString &wanted)
{
    if (!triggerSource_)
        return;
    triggerSource_->blockSignals(true);
    triggerSource_->clear();
    obs_enum_scenes(add_trigger_source, triggerSource_);
    obs_enum_sources(add_trigger_source, triggerSource_);
    triggerSource_->blockSignals(false);
    const int index = triggerSource_->findData(wanted);
    if (index >= 0)
        triggerSource_->setCurrentIndex(index);
    else if (triggerSource_->count())
        triggerSource_->setCurrentIndex(0);
}

static void add_trigger_filter(obs_source_t *, obs_source_t *filter, void *data)
{
    auto *combo = static_cast<QComboBox *>(data);
    if (combo && filter)
        combo->addItem(QString::fromUtf8(obs_source_get_name(filter)),
                       QString::fromUtf8(obs_source_get_name(filter)));
}

void NodeSettingsDialog::populateTriggerFilters(const QString &wanted)
{
    if (!triggerFilter_ || !triggerSource_)
        return;
    triggerFilter_->blockSignals(true);
    triggerFilter_->clear();
    const QString parentName = triggerSource_->currentData().toString().isEmpty()
                                   ? triggerSource_->currentText().trimmed()
                                   : triggerSource_->currentData().toString();
    if (!parentName.isEmpty()) {
        obs_source_t *parent = obs_get_source_by_name(parentName.toUtf8().constData());
        if (parent) {
            obs_source_enum_filters(parent, add_trigger_filter, triggerFilter_);
            obs_source_release(parent);
        }
    }
    triggerFilter_->blockSignals(false);
    const int index = triggerFilter_->findData(wanted);
    if (index >= 0)
        triggerFilter_->setCurrentIndex(index);
    else if (triggerFilter_->count())
        triggerFilter_->setCurrentIndex(0);
}

bool NodeSettingsDialog::applyTrigger()
{
    workflow_node_t *wf = node_->workflowNode();
    workflow_trigger_ref_t &trigger = wf->trigger;
    const auto type = (workflow_trigger_type_t)triggerAction_->currentData().toInt();
    trigger.type = type;
    const QString action = type == WORKFLOW_TRIGGER_FRONTEND_ACTION
        ? (triggerActionValue_ ? triggerActionValue_->currentText() : QString())
        : QString::fromUtf8(workflow_trigger_type_name(type));
    settings_copy_text(trigger.action, WORKFLOW_MAX_NAME, action);
    if (triggerSource_) {
        const QString source = triggerSource_->currentData().toString().isEmpty()
            ? triggerSource_->currentText().trimmed() : triggerSource_->currentData().toString();
        settings_copy_text(trigger.scene_name, WORKFLOW_MAX_NAME, source);
    }
    if (triggerFilter_)
        settings_copy_text(trigger.filter_name, WORKFLOW_MAX_NAME, triggerFilter_->currentData().toString());
    if (triggerState_)
        trigger.state = (workflow_trigger_state_t)triggerState_->currentData().toInt();
    trigger.audio_track = triggerAudioTrack_ ? triggerAudioTrack_->value() : 0;
    if (triggerHotkey_)
        settings_copy_text(trigger.hotkey, WORKFLOW_MAX_NAME, triggerHotkey_->text().trimmed());
    if (triggerSettingName_)
        settings_copy_text(trigger.setting_name, WORKFLOW_MAX_NAME, triggerSettingName_->text().trimmed());
    if (triggerValue_)
        settings_copy_text(trigger.value, WORKFLOW_MAX_VALUE, triggerValue_->text());
    if (triggerMatch_)
        settings_copy_text(trigger.match, WORKFLOW_MAX_VALUE, triggerMatch_->text());
    trigger.udp_port = triggerUdpPort_ ? (uint16_t)triggerUdpPort_->value() : 0;

    if (type == WORKFLOW_TRIGGER_FILTER_ENABLE && triggerSource_ && triggerFilter_) {
        const QString source = settings_read_text(trigger.scene_name);
        const QString filterName = settings_read_text(trigger.filter_name);
        if (!source.isEmpty() && !filterName.isEmpty()) {
            obs_source_t *parent = obs_get_source_by_name(source.toUtf8().constData());
            if (parent) {
                obs_source_t *filter = obs_source_get_filter_by_name(parent, filterName.toUtf8().constData());
                if (filter) {
                    settings_copy_text(trigger.filter_id, WORKFLOW_MAX_NAME,
                                       QString::fromUtf8(obs_source_get_id(filter)));
                    obs_source_release(filter);
                }
                obs_source_release(parent);
            }
        }
    }
    startActions_->apply(wf->simultaneous_node_count, wf->simultaneous_node_ids);
    wf->simultaneous_actions_mode = WORKFLOW_OVERRIDE;
    return true;
}
