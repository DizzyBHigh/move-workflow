#include "workflow-node-settings.h"

#include "workflow-action-list.h"
#include "workflow-debug.h"
#include "workflow-engine-service.h"
#include "workflow-node-settings-common.h"
#include "workflow-persistence.h"

#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QVBoxLayout>

void NodeSettingsDialog::buildTriggerEditor(QVBoxLayout *layout, QVBoxLayout *contentLayout)
{
    triggerAction_ = new QComboBox(this);
    const QList<QPair<QString, workflow_trigger_type_t>> types = {
        {"None", WORKFLOW_TRIGGER_NONE}, {"Frontend Action", WORKFLOW_TRIGGER_FRONTEND_ACTION},
        {"Source Visibility", WORKFLOW_TRIGGER_SOURCE_VISIBILITY}, {"Source Mute", WORKFLOW_TRIGGER_SOURCE_MUTE},
        {"Source Audio Track", WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK}, {"Source Hotkey", WORKFLOW_TRIGGER_SOURCE_HOTKEY},
        {"Filter Enable", WORKFLOW_TRIGGER_FILTER_ENABLE}, {"Frontend Hotkey", WORKFLOW_TRIGGER_FRONTEND_HOTKEY},
        {"Setting", WORKFLOW_TRIGGER_SETTING}, {"UDP Packet", WORKFLOW_TRIGGER_UDP_PACKET},
        {"WebSocket Request", WORKFLOW_TRIGGER_WEBSOCKET_REQUEST}, {"WebSocket Event", WORKFLOW_TRIGGER_WEBSOCKET_EVENT}};
    for (const auto &entry : types)
        triggerAction_->addItem(entry.first, (int)entry.second);
    const int index = triggerAction_->findData((int)node_->workflowNode()->trigger.type);
    triggerAction_->setCurrentIndex(index >= 0 ? index : 0);
    layout->addWidget(new QLabel("Trigger", this));
    layout->addWidget(triggerAction_);
    auto *testButton = new QPushButton("▶ Test Workflow From This Trigger", this);
    testButton->setToolTip("Execute this workflow branch without waiting for the real trigger.");
    layout->addWidget(testButton);
    connect(testButton, &QPushButton::clicked, this, [this] {
        const workflow_manager_t *manager = workflow_persistence_manager();
        const workflow_t *workflow = manager ? workflow_manager_selected_const(manager) : nullptr;
        if (!workflow) {
            blog(LOG_WARNING, "[Move Workflow] Trigger test could not find the selected workflow.");
            return;
        }
        workflow_debug_log("Manual test requested from trigger node: %s", node_->id().toUtf8().constData());
        const bool started = workflow_engine_service_test_node(workflow->id, node_->workflowNode()->id);
        if (!started)
            blog(LOG_WARNING, "[Move Workflow] Trigger test could not start.");
    });
    triggerSettingsBox_ = new QGroupBox("Trigger Settings", this);
    triggerSettingsLayout_ = new QVBoxLayout(triggerSettingsBox_);
    layout->addWidget(triggerSettingsBox_);
    connect(triggerAction_, &QComboBox::currentIndexChanged, this,
            [this](int) { rebuildTriggerSettings(); });
    rebuildTriggerSettings();

    startActions_ = new WorkflowActionList("Start Actions",
        "These actions start when this Trigger fires. Multiple actions run in parallel.",
        node_, nodes_, node_->workflowNode()->simultaneous_node_ids,
        node_->workflowNode()->simultaneous_node_count, this);
    contentLayout->addWidget(startActions_);
    auto *hint = new QLabel("The trigger identifies the OBS event that starts this workflow branch. A Trigger Node may be connected anywhere in the graph.", this);
    hint->setWordWrap(true);
    hint->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    contentLayout->addWidget(hint);
}

void NodeSettingsDialog::clearTriggerSettings()
{
    while (triggerSettingsLayout_ && triggerSettingsLayout_->count()) {
        QLayoutItem *item = triggerSettingsLayout_->takeAt(0);
        if (QWidget *widget = item->widget())
            widget->deleteLater();
        delete item;
    }
    triggerSource_ = nullptr; triggerFilter_ = nullptr; triggerState_ = nullptr;
    triggerAudioTrack_ = nullptr; triggerUdpPort_ = nullptr;
    triggerActionValue_ = nullptr; triggerHotkey_ = nullptr;
    triggerSettingName_ = nullptr; triggerValue_ = nullptr; triggerMatch_ = nullptr;
}

static QComboBox *trigger_state(QWidget *parent, workflow_trigger_state_t state)
{
    auto *combo = new QComboBox(parent);
    combo->addItem("Enabled", (int)WORKFLOW_TRIGGER_STATE_ENABLED);
    combo->addItem("Disabled", (int)WORKFLOW_TRIGGER_STATE_DISABLED);
    combo->setCurrentIndex(combo->findData((int)state));
    return combo;
}

static QLineEdit *trigger_edit(QWidget *parent, const QString &value, const QString &hint)
{
    auto *edit = new QLineEdit(value, parent);
    edit->setPlaceholderText(hint);
    return edit;
}

void NodeSettingsDialog::addTriggerRow(const QString &label, QWidget *widget)
{
    auto *row = new QHBoxLayout;
    row->addWidget(new QLabel(label, triggerSettingsBox_));
    row->addWidget(widget, 1);
    triggerSettingsLayout_->addLayout(row);
}

void NodeSettingsDialog::rebuildTriggerSettings()
{
    clearTriggerSettings();
    const workflow_trigger_ref_t &trigger = node_->workflowNode()->trigger;
    const auto type = (workflow_trigger_type_t)triggerAction_->currentData().toInt();
    switch (type) {
    case WORKFLOW_TRIGGER_FRONTEND_ACTION:
        triggerActionValue_ = new QComboBox(triggerSettingsBox_);
        triggerActionValue_->setEditable(true);
        triggerActionValue_->addItems({"Start Streaming", "Stop Streaming", "Start Recording", "Stop Recording", "Pause Recording", "Resume Recording", "Toggle Studio Mode", "Start Replay Buffer", "Stop Replay Buffer", "Save Replay Buffer"});
        triggerActionValue_->setCurrentText(settings_read_text(trigger.action));
        addTriggerRow("Action", triggerActionValue_); break;
    case WORKFLOW_TRIGGER_SOURCE_VISIBILITY: buildSourceStateSettings("Visibility", trigger.state); break;
    case WORKFLOW_TRIGGER_SOURCE_MUTE: buildSourceStateSettings("Mute State", trigger.state); break;
    case WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK: buildSourceAudioTrackSettings(trigger); break;
    case WORKFLOW_TRIGGER_SOURCE_HOTKEY: buildSourceHotkeySettings(trigger); break;
    case WORKFLOW_TRIGGER_FILTER_ENABLE: buildFilterEnableSettings(trigger); break;
    case WORKFLOW_TRIGGER_FRONTEND_HOTKEY:
        triggerHotkey_ = trigger_edit(triggerSettingsBox_, settings_read_text(trigger.hotkey), "Hotkey name");
        addTriggerRow("Hotkey", triggerHotkey_); break;
    case WORKFLOW_TRIGGER_SETTING: buildSettingSettings(trigger); break;
    case WORKFLOW_TRIGGER_UDP_PACKET:
        triggerUdpPort_ = new QSpinBox(triggerSettingsBox_); triggerUdpPort_->setRange(1, 65535);
        triggerUdpPort_->setValue(trigger.udp_port ? trigger.udp_port : 9000);
        addTriggerRow("Port", triggerUdpPort_);
        triggerMatch_ = trigger_edit(triggerSettingsBox_, settings_read_text(trigger.match), "Packet text or pattern");
        addTriggerRow("Match", triggerMatch_); break;
    case WORKFLOW_TRIGGER_WEBSOCKET_REQUEST:
    case WORKFLOW_TRIGGER_WEBSOCKET_EVENT:
        triggerMatch_ = trigger_edit(triggerSettingsBox_, settings_read_text(trigger.match), "Request/event match");
        addTriggerRow("Match", triggerMatch_); break;
    default: triggerSettingsLayout_->addWidget(new QLabel("No trigger settings are required.", triggerSettingsBox_));
    }
}

void NodeSettingsDialog::buildSourceStateSettings(const QString &label, workflow_trigger_state_t state)
{
    triggerSource_ = new QComboBox(triggerSettingsBox_); settings_searchable(triggerSource_);
    populateTriggerSources(settings_read_text(node_->workflowNode()->trigger.scene_name));
    addTriggerRow("Source", triggerSource_);
    triggerState_ = trigger_state(triggerSettingsBox_, state);
    addTriggerRow(label, triggerState_);
}

void NodeSettingsDialog::buildSourceAudioTrackSettings(const workflow_trigger_ref_t &trigger)
{
    buildSourceStateSettings("Enabled", trigger.state);
    triggerAudioTrack_ = new QSpinBox(triggerSettingsBox_);
    triggerAudioTrack_->setRange(1, 32); triggerAudioTrack_->setValue(trigger.audio_track ? trigger.audio_track : 1);
    addTriggerRow("Track", triggerAudioTrack_);
}

void NodeSettingsDialog::buildSourceHotkeySettings(const workflow_trigger_ref_t &trigger)
{
    triggerSource_ = new QComboBox(triggerSettingsBox_); settings_searchable(triggerSource_);
    populateTriggerSources(settings_read_text(trigger.scene_name)); addTriggerRow("Source", triggerSource_);
    triggerHotkey_ = trigger_edit(triggerSettingsBox_, settings_read_text(trigger.hotkey), "Hotkey name");
    addTriggerRow("Hotkey", triggerHotkey_);
}

void NodeSettingsDialog::buildFilterEnableSettings(const workflow_trigger_ref_t &trigger)
{
    triggerSource_ = new QComboBox(triggerSettingsBox_); triggerFilter_ = new QComboBox(triggerSettingsBox_);
    settings_searchable(triggerSource_); settings_searchable(triggerFilter_);
    populateTriggerSources(settings_read_text(trigger.scene_name)); addTriggerRow("Source", triggerSource_);
    populateTriggerFilters(settings_read_text(trigger.filter_name)); addTriggerRow("Filter", triggerFilter_);
    triggerState_ = trigger_state(triggerSettingsBox_, trigger.state); addTriggerRow("State", triggerState_);
    connect(triggerSource_, &QComboBox::currentIndexChanged, this, [this] { populateTriggerFilters(); });
}

void NodeSettingsDialog::buildSettingSettings(const workflow_trigger_ref_t &trigger)
{
    triggerSource_ = new QComboBox(triggerSettingsBox_); settings_searchable(triggerSource_);
    populateTriggerSources(settings_read_text(trigger.scene_name)); addTriggerRow("Source", triggerSource_);
    triggerSettingName_ = trigger_edit(triggerSettingsBox_, settings_read_text(trigger.setting_name), "Setting name");
    triggerValue_ = trigger_edit(triggerSettingsBox_, settings_read_text(trigger.value), "Expected value");
    addTriggerRow("Setting", triggerSettingName_); addTriggerRow("Value", triggerValue_);
}
