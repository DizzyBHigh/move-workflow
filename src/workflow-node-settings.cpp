#include "workflow-node-dialog.h"

#include "workflow-action-list.h"
#include "workflow-model.h"

#include <obs-frontend-api.h>
#include <obs.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpinBox>
#include <QVBoxLayout>

#include <cstring>

namespace {

static void copy_text(char *destination, size_t capacity, const QString &value)
{
    if (!destination || capacity == 0)
        return;
    const QByteArray bytes = value.toUtf8();
    std::strncpy(destination, bytes.constData(), capacity - 1);
    destination[capacity - 1] = '\0';
}

static QString read_text(const char *value)
{
    return value ? QString::fromUtf8(value) : QString();
}

static void configure_searchable_combo(QComboBox *combo)
{
    if (!combo || combo->isEditable())
        return;
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::NoInsert);
    auto *completer = new QCompleter(combo->model(), combo);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    combo->setCompleter(completer);
}

static workflow_move_kind_t workflow_kind_from_filter_id(const char *id)
{
    if (!id)
        return WORKFLOW_MOVE_ACTION;
    if (std::strcmp(id, "move_source_filter") == 0)
        return WORKFLOW_MOVE_SOURCE;
    if (std::strcmp(id, "move_source_swap_filter") == 0)
        return WORKFLOW_MOVE_SWAP;
    if (std::strcmp(id, "move_value_filter") == 0)
        return WORKFLOW_MOVE_VALUE;
    return WORKFLOW_MOVE_ACTION;
}

static bool is_supported_move_filter(const char *id)
{
    return id && (std::strcmp(id, "move_action_filter") == 0 ||
                  std::strcmp(id, "move_source_filter") == 0 ||
                  std::strcmp(id, "move_source_swap_filter") == 0 ||
                  std::strcmp(id, "move_value_filter") == 0);
}

class NodeSettingsDialog final : public QDialog {
public:
    NodeSettingsDialog(NodeItem *node, const QList<NodeItem *> &nodes, QWidget *parent = nullptr)
        : QDialog(parent), node_(node), nodes_(nodes)
    {
        setWindowTitle(QString("Node Settings - %1").arg(node ? node->nodeName() : "Node"));
        resize(560, 800);
        setMinimumSize(500, 400);

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(8, 8, 8, 8);
        auto *contentArea = new QScrollArea(this);
        contentArea->setWidgetResizable(true);
        contentArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        contentArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        auto *content = new QWidget;
        auto *layout = new QVBoxLayout(content);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(8);

        const bool isTrigger = node && node->workflowNode()->type == WORKFLOW_NODE_TRIGGER;
        auto *nodeBox = new QGroupBox(isTrigger ? "Trigger Node" : "Node", content);
        auto *nodeLayout = new QVBoxLayout(nodeBox);
        name_ = new QLineEdit(node ? node->nodeName() : QString(), nodeBox);
        nodeLayout->addWidget(new QLabel("Name", nodeBox));
        nodeLayout->addWidget(name_);
        layout->addWidget(nodeBox);
        if (isTrigger)
            buildTriggerEditor(nodeLayout, layout);
        else
            buildActionEditor(content, layout);

        layout->addStretch(1);
        contentArea->setWidget(content);
        root->addWidget(contentArea, 1);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, [this] {
            if (apply())
                accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        root->addWidget(buttons);
    }

private:
    bool apply()
    {
        if (!node_)
            return false;
        const QString name = name_->text().trimmed();
        if (name.isEmpty())
            return false;
        workflow_node_t *wf = node_->workflowNode();
        copy_text(wf->name, WORKFLOW_MAX_NAME, name);
        if (wf->type == WORKFLOW_NODE_TRIGGER)
            return applyTrigger();

        const QString parentName = source_->currentData().toString().isEmpty()
                                        ? source_->currentText().trimmed()
                                        : source_->currentData().toString();
        const QString filterName = filter_->currentData().toString();
        if (parentName.isEmpty() || filterName.isEmpty())
            return false;
        obs_source_t *parent = obs_get_source_by_name(parentName.toUtf8().constData());
        if (!parent)
            return false;
        obs_source_t *filter = obs_source_get_filter_by_name(parent, filterName.toUtf8().constData());
        if (!filter) {
            obs_source_release(parent);
            return false;
        }
        const char *filterId = obs_source_get_id(filter);
        if (!is_supported_move_filter(filterId)) {
            obs_source_release(filter);
            obs_source_release(parent);
            return false;
        }
        copy_text(wf->action.scene_name, WORKFLOW_MAX_NAME, parentName);
        wf->action.source_name[0] = '\0';
        copy_text(wf->action.filter_name, WORKFLOW_MAX_NAME, filterName);
        copy_text(wf->action.filter_id, WORKFLOW_MAX_NAME, QString::fromUtf8(filterId));
        wf->action.kind = workflow_kind_from_filter_id(filterId);
        wf->start_delay.mode = startDelayDefault_->isChecked() ? WORKFLOW_USE_EXISTING : WORKFLOW_OVERRIDE;
        wf->start_delay.delay_ms = (uint64_t)startDelayMs_->value();
        wf->duration.mode = durationDefault_->isChecked() ? WORKFLOW_USE_EXISTING : WORKFLOW_OVERRIDE;
        wf->duration.duration_ms = (uint64_t)durationMs_->value();
        wf->end_delay.mode = endDelayDefault_->isChecked() ? WORKFLOW_USE_EXISTING : WORKFLOW_OVERRIDE;
        wf->end_delay.delay_ms = (uint64_t)endDelayMs_->value();
        wf->simultaneous_actions_mode = WORKFLOW_OVERRIDE;
        wf->end_actions_mode = WORKFLOW_OVERRIDE;
        wf->next_actions_mode = WORKFLOW_OVERRIDE;
        simultaneous_->apply(wf->simultaneous_node_count, wf->simultaneous_node_ids);
        endActions_->apply(wf->end_node_count, wf->end_node_ids);
        nextActions_->apply(wf->next_node_count, wf->next_node_ids);
        obs_source_release(filter);
        obs_source_release(parent);
        return true;
    }

    void buildTriggerEditor(QVBoxLayout *layout, QVBoxLayout *contentLayout)
    {
        triggerAction_ = new QComboBox(this);
        const QList<QPair<QString, workflow_trigger_type_t>> triggerTypes = {
            {"None", WORKFLOW_TRIGGER_NONE}, {"Frontend Action", WORKFLOW_TRIGGER_FRONTEND_ACTION},
            {"Source Visibility", WORKFLOW_TRIGGER_SOURCE_VISIBILITY}, {"Source Mute", WORKFLOW_TRIGGER_SOURCE_MUTE},
            {"Source Audio Track", WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK}, {"Source Hotkey", WORKFLOW_TRIGGER_SOURCE_HOTKEY},
            {"Filter Enable", WORKFLOW_TRIGGER_FILTER_ENABLE}, {"Frontend Hotkey", WORKFLOW_TRIGGER_FRONTEND_HOTKEY},
            {"Setting", WORKFLOW_TRIGGER_SETTING}, {"UDP Packet", WORKFLOW_TRIGGER_UDP_PACKET},
            {"WebSocket Request", WORKFLOW_TRIGGER_WEBSOCKET_REQUEST}, {"WebSocket Event", WORKFLOW_TRIGGER_WEBSOCKET_EVENT},
        };
        for (const auto &entry : triggerTypes)
            triggerAction_->addItem(entry.first, (int)entry.second);
        workflow_trigger_type_t storedType = node_->workflowNode()->trigger.type;
        if (storedType == WORKFLOW_TRIGGER_NONE) {
            const QString legacy = read_text(node_->workflowNode()->trigger.action);
            for (int i = 0; i < triggerAction_->count(); ++i) {
                if (triggerAction_->itemText(i).compare(legacy, Qt::CaseInsensitive) == 0) {
                    storedType = (workflow_trigger_type_t)triggerAction_->itemData(i).toInt();
                    break;
                }
            }
        }
        const int storedIndex = triggerAction_->findData((int)storedType);
        triggerAction_->setCurrentIndex(storedIndex >= 0 ? storedIndex : 0);
        layout->addWidget(new QLabel("Trigger", this));
        layout->addWidget(triggerAction_);
        triggerSettingsBox_ = new QGroupBox("Trigger Settings", this);
        triggerSettingsLayout_ = new QVBoxLayout(triggerSettingsBox_);
        layout->addWidget(triggerSettingsBox_);
        connect(triggerAction_, &QComboBox::currentIndexChanged, this, [this](int) { rebuildTriggerSettings(); });
        rebuildTriggerSettings();
        startActions_ = new WorkflowActionList(
            "Start Actions",
            "These actions start when this Trigger fires. Multiple actions run in parallel.",
            node_, nodes_, node_->workflowNode()->simultaneous_node_ids,
            node_->workflowNode()->simultaneous_node_count, this);
        contentLayout->addWidget(startActions_);
        auto *hint = new QLabel("The trigger identifies the OBS event that starts this workflow branch. A Trigger Node may be connected anywhere in the graph.", this);
        hint->setWordWrap(true);
        hint->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        contentLayout->addWidget(hint);
    }

    void clearTriggerSettings()
    {
        if (!triggerSettingsLayout_)
            return;
        while (QLayoutItem *item = triggerSettingsLayout_->takeAt(0)) {
            if (QWidget *widget = item->widget())
                widget->deleteLater();
            delete item;
        }
        triggerSource_ = nullptr;
        triggerFilter_ = nullptr;
        triggerState_ = nullptr;
        triggerAudioTrack_ = nullptr;
        triggerActionValue_ = nullptr;
        triggerHotkey_ = nullptr;
        triggerSettingName_ = nullptr;
        triggerValue_ = nullptr;
        triggerMatch_ = nullptr;
        triggerUdpPort_ = nullptr;
    }

    QComboBox *makeTriggerStateCombo(QWidget *parent, workflow_trigger_state_t state)
    {
        auto *combo = new QComboBox(parent);
        combo->addItem("Enabled", (int)WORKFLOW_TRIGGER_STATE_ENABLED);
        combo->addItem("Disabled", (int)WORKFLOW_TRIGGER_STATE_DISABLED);
        const int index = combo->findData((int)state);
        combo->setCurrentIndex(index >= 0 ? index : 0);
        return combo;
    }

    QLineEdit *makeTriggerLineEdit(QWidget *parent, const QString &value, const QString &placeholder = QString())
    {
        auto *edit = new QLineEdit(value, parent);
        edit->setPlaceholderText(placeholder);
        return edit;
    }

    void rebuildTriggerSettings()
    {
        clearTriggerSettings();
        if (!triggerSettingsLayout_)
            return;
        const workflow_trigger_ref_t &trigger = node_->workflowNode()->trigger;
        const workflow_trigger_type_t type = (workflow_trigger_type_t)triggerAction_->currentData().toInt();
        switch (type) {
        case WORKFLOW_TRIGGER_FRONTEND_ACTION:
            triggerActionValue_ = new QComboBox(triggerSettingsBox_);
            triggerActionValue_->setEditable(true);
            triggerActionValue_->addItems({"Start Streaming", "Stop Streaming", "Start Recording", "Stop Recording", "Pause Recording", "Resume Recording", "Toggle Studio Mode", "Start Replay Buffer", "Stop Replay Buffer", "Save Replay Buffer", "Enable Preview", "Disable Preview"});
            triggerActionValue_->setCurrentText(read_text(trigger.action));
            addTriggerRow("Action", triggerActionValue_);
            break;
        case WORKFLOW_TRIGGER_SOURCE_VISIBILITY: buildSourceStateSettings("Visibility", trigger.state); break;
        case WORKFLOW_TRIGGER_SOURCE_MUTE: buildSourceStateSettings("Mute State", trigger.state); break;
        case WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK: buildSourceAudioTrackSettings(trigger); break;
        case WORKFLOW_TRIGGER_SOURCE_HOTKEY: buildSourceHotkeySettings(trigger); break;
        case WORKFLOW_TRIGGER_FILTER_ENABLE: buildFilterEnableSettings(trigger); break;
        case WORKFLOW_TRIGGER_FRONTEND_HOTKEY:
            triggerHotkey_ = makeTriggerLineEdit(triggerSettingsBox_, read_text(trigger.hotkey), "Hotkey or OBS hotkey name");
            addTriggerRow("Hotkey", triggerHotkey_);
            break;
        case WORKFLOW_TRIGGER_SETTING: buildSettingSettings(trigger); break;
        case WORKFLOW_TRIGGER_UDP_PACKET:
            triggerUdpPort_ = new QSpinBox(triggerSettingsBox_);
            triggerUdpPort_->setRange(1, 65535);
            triggerUdpPort_->setValue(trigger.udp_port > 0 ? trigger.udp_port : 9000);
            addTriggerRow("Port", triggerUdpPort_);
            triggerMatch_ = makeTriggerLineEdit(triggerSettingsBox_, read_text(trigger.match), "Packet text or pattern");
            addTriggerRow("Match", triggerMatch_);
            break;
        case WORKFLOW_TRIGGER_WEBSOCKET_REQUEST:
        case WORKFLOW_TRIGGER_WEBSOCKET_EVENT:
            triggerMatch_ = makeTriggerLineEdit(triggerSettingsBox_, read_text(trigger.match), "Request/event name or match pattern");
            addTriggerRow("Match", triggerMatch_);
            break;
        default:
            triggerSettingsLayout_->addWidget(new QLabel("No trigger settings are required.", triggerSettingsBox_));
            break;
        }
    }

    void addTriggerRow(const QString &label, QWidget *widget)
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel(label, triggerSettingsBox_));
        row->addWidget(widget, 1);
        triggerSettingsLayout_->addLayout(row);
    }

    void buildSourceStateSettings(const QString &stateLabel, workflow_trigger_state_t state)
    {
        triggerSource_ = new QComboBox(triggerSettingsBox_);
        configure_searchable_combo(triggerSource_);
        populateTriggerSources(read_text(node_->workflowNode()->trigger.scene_name));
        addTriggerRow("Source", triggerSource_);
        triggerState_ = makeTriggerStateCombo(triggerSettingsBox_, state);
        addTriggerRow(stateLabel, triggerState_);
    }

    void buildSourceAudioTrackSettings(const workflow_trigger_ref_t &trigger)
    {
        triggerSource_ = new QComboBox(triggerSettingsBox_);
        configure_searchable_combo(triggerSource_);
        populateTriggerSources(read_text(trigger.scene_name));
        addTriggerRow("Source", triggerSource_);
        triggerAudioTrack_ = new QSpinBox(triggerSettingsBox_);
        triggerAudioTrack_->setRange(1, 32);
        triggerAudioTrack_->setValue(trigger.audio_track > 0 ? trigger.audio_track : 1);
        addTriggerRow("Track", triggerAudioTrack_);
        triggerState_ = makeTriggerStateCombo(triggerSettingsBox_, trigger.state);
        addTriggerRow("Enabled", triggerState_);
    }

    void buildSourceHotkeySettings(const workflow_trigger_ref_t &trigger)
    {
        triggerSource_ = new QComboBox(triggerSettingsBox_);
        configure_searchable_combo(triggerSource_);
        populateTriggerSources(read_text(trigger.scene_name));
        addTriggerRow("Source", triggerSource_);
        triggerHotkey_ = makeTriggerLineEdit(triggerSettingsBox_, read_text(trigger.hotkey), "Hotkey name");
        addTriggerRow("Hotkey", triggerHotkey_);
    }

    void buildFilterEnableSettings(const workflow_trigger_ref_t &trigger)
    {
        triggerSource_ = new QComboBox(triggerSettingsBox_);
        triggerFilter_ = new QComboBox(triggerSettingsBox_);
        configure_searchable_combo(triggerSource_);
        configure_searchable_combo(triggerFilter_);
        populateTriggerSources(read_text(trigger.scene_name));
        addTriggerRow("Source", triggerSource_);
        populateTriggerFilters(read_text(trigger.filter_name));
        addTriggerRow("Filter", triggerFilter_);
        triggerState_ = makeTriggerStateCombo(triggerSettingsBox_, trigger.state);
        addTriggerRow("State", triggerState_);
        connect(triggerSource_, &QComboBox::currentIndexChanged, this, [this] { populateTriggerFilters(); });
    }

    void buildSettingSettings(const workflow_trigger_ref_t &trigger)
    {
        triggerSource_ = new QComboBox(triggerSettingsBox_);
        configure_searchable_combo(triggerSource_);
        populateTriggerSources(read_text(trigger.scene_name));
        addTriggerRow("Source", triggerSource_);
        triggerSettingName_ = makeTriggerLineEdit(triggerSettingsBox_, read_text(trigger.setting_name), "Setting name");
        addTriggerRow("Setting", triggerSettingName_);
        triggerValue_ = makeTriggerLineEdit(triggerSettingsBox_, read_text(trigger.value), "Expected value");
        addTriggerRow("Value", triggerValue_);
    }

    bool applyTrigger()
    {
        workflow_node_t *wf = node_->workflowNode();
        workflow_trigger_ref_t &trigger = wf->trigger;
        const workflow_trigger_type_t type = (workflow_trigger_type_t)triggerAction_->currentData().toInt();
        trigger.type = type;
        const QString actionName = type == WORKFLOW_TRIGGER_FRONTEND_ACTION
                                        ? (triggerActionValue_ ? triggerActionValue_->currentText() : QString())
                                        : QString::fromUtf8(workflow_trigger_type_name(type));
        copy_text(trigger.action, WORKFLOW_MAX_NAME, actionName);
        const QString sourceName = triggerSource_
                                       ? (triggerSource_->currentData().toString().isEmpty()
                                              ? triggerSource_->currentText().trimmed()
                                              : triggerSource_->currentData().toString())
                                       : QString();
        if (triggerSource_)
            copy_text(trigger.scene_name, WORKFLOW_MAX_NAME, sourceName);
        else
            trigger.scene_name[0] = '\0';
        if (triggerFilter_)
            copy_text(trigger.filter_name, WORKFLOW_MAX_NAME, triggerFilter_->currentData().toString());
        else
            trigger.filter_name[0] = '\0';
        if (triggerState_)
            trigger.state = (workflow_trigger_state_t)triggerState_->currentData().toInt();
        else
            trigger.state = WORKFLOW_TRIGGER_STATE_ENABLED;
        trigger.audio_track = triggerAudioTrack_ ? triggerAudioTrack_->value() : 0;
        if (triggerHotkey_)
            copy_text(trigger.hotkey, WORKFLOW_MAX_NAME, triggerHotkey_->text().trimmed());
        else
            trigger.hotkey[0] = '\0';
        if (triggerSettingName_)
            copy_text(trigger.setting_name, WORKFLOW_MAX_NAME, triggerSettingName_->text().trimmed());
        else
            trigger.setting_name[0] = '\0';
        if (triggerValue_)
            copy_text(trigger.value, WORKFLOW_MAX_VALUE, triggerValue_->text());
        else
            trigger.value[0] = '\0';
        if (triggerMatch_)
            copy_text(trigger.match, WORKFLOW_MAX_VALUE, triggerMatch_->text());
        else
            trigger.match[0] = '\0';
        trigger.udp_port = triggerUdpPort_ ? (uint16_t)triggerUdpPort_->value() : 0;

        if (type == WORKFLOW_TRIGGER_FILTER_ENABLE) {
            const QString source = read_text(trigger.scene_name);
            const QString filterName = read_text(trigger.filter_name);
            if (source.isEmpty() || filterName.isEmpty())
                return false;
            obs_source_t *parent = obs_get_source_by_name(source.toUtf8().constData());
            if (!parent)
                return false;
            obs_source_t *filter = obs_source_get_filter_by_name(parent, filterName.toUtf8().constData());
            if (!filter) {
                obs_source_release(parent);
                return false;
            }
            copy_text(trigger.filter_id, WORKFLOW_MAX_NAME, QString::fromUtf8(obs_source_get_id(filter)));
            obs_source_release(filter);
            obs_source_release(parent);
        } else {
            trigger.filter_id[0] = '\0';
        }
        wf->simultaneous_actions_mode = WORKFLOW_OVERRIDE;
        startActions_->apply(wf->simultaneous_node_count, wf->simultaneous_node_ids);
        return true;
    }

    static bool addTriggerSource(void *data, obs_source_t *source)
    {
        auto *combo = static_cast<QComboBox *>(data);
        if (!combo || !source)
            return true;
        const QString name = QString::fromUtf8(obs_source_get_name(source));
        if (combo->findData(name) < 0)
            combo->addItem(name, name);
        return true;
    }

    void populateTriggerSources(const QString &wanted = QString())
    {
        if (!triggerSource_)
            return;
        configure_searchable_combo(triggerSource_);
        triggerSource_->blockSignals(true);
        triggerSource_->clear();
        obs_enum_scenes(addTriggerSource, triggerSource_);
        obs_enum_sources(addTriggerSource, triggerSource_);
        triggerSource_->blockSignals(false);
        const int index = triggerSource_->findData(wanted);
        if (index >= 0)
            triggerSource_->setCurrentIndex(index);
        else if (triggerSource_->count() > 0)
            triggerSource_->setCurrentIndex(0);
    }

    static void addTriggerFilter(obs_source_t *parent, obs_source_t *filter, void *param)
    {
        Q_UNUSED(parent);
        auto *combo = static_cast<QComboBox *>(param);
        if (!combo || !filter)
            return;
        const QString name = QString::fromUtf8(obs_source_get_name(filter));
        combo->addItem(name, name);
    }

    void populateTriggerFilters(const QString &wanted = QString())
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
                obs_source_enum_filters(parent, addTriggerFilter, triggerFilter_);
                obs_source_release(parent);
            }
        }
        triggerFilter_->blockSignals(false);
        const int index = triggerFilter_->findData(wanted);
        if (index >= 0)
            triggerFilter_->setCurrentIndex(index);
        else if (triggerFilter_->count() > 0)
            triggerFilter_->setCurrentIndex(0);
    }

    void buildActionEditor(QWidget *parent, QVBoxLayout *layout)
    {
        auto *targetBox = new QGroupBox("Existing Move / Swap / Value Action", parent);
        auto *targetLayout = new QVBoxLayout(targetBox);
        source_ = new QComboBox(targetBox);
        filter_ = new QComboBox(targetBox);
        targetLayout->addWidget(new QLabel("Source", targetBox));
        targetLayout->addWidget(source_);
        targetLayout->addWidget(new QLabel("Filter", targetBox));
        targetLayout->addWidget(filter_);
        layout->addWidget(targetBox);
        configure_searchable_combo(source_);
        populateSources(read_text(node_->workflowNode()->action.scene_name));
        connect(source_, &QComboBox::currentIndexChanged, this, [this] { populateFilters(); });
        populateFilters(read_text(node_->workflowNode()->action.filter_name));

        auto *timingBox = new QGroupBox("Timing", parent);
        auto *timingLayout = new QVBoxLayout(timingBox);
        startDelayMs_ = makeMilliseconds(timingBox);
        durationMs_ = makeMilliseconds(timingBox);
        endDelayMs_ = makeMilliseconds(timingBox);
        startDelayDefault_ = new QCheckBox("Use default", timingBox);
        durationDefault_ = new QCheckBox("Use default", timingBox);
        endDelayDefault_ = new QCheckBox("Use default", timingBox);
        if (node_) {
            const workflow_node_t *wf = node_->workflowNode();
            startDelayMs_->setValue((int)wf->start_delay.delay_ms);
            durationMs_->setValue((int)wf->duration.duration_ms);
            endDelayMs_->setValue((int)wf->end_delay.delay_ms);
            startDelayDefault_->setChecked(wf->start_delay.mode == WORKFLOW_USE_EXISTING);
            durationDefault_->setChecked(wf->duration.mode == WORKFLOW_USE_EXISTING);
            endDelayDefault_->setChecked(wf->end_delay.mode == WORKFLOW_USE_EXISTING);
        }
        addSpinRow(timingLayout, "Start Delay", startDelayMs_, startDelayDefault_);
        addSpinRow(timingLayout, "Duration", durationMs_, durationDefault_);
        addSpinRow(timingLayout, "End Delay", endDelayMs_, endDelayDefault_);
        connectDefaultToggle(startDelayDefault_, startDelayMs_);
        connectDefaultToggle(durationDefault_, durationMs_);
        connectDefaultToggle(endDelayDefault_, endDelayMs_);
        layout->addWidget(timingBox);

        const workflow_node_t *wf = node_->workflowNode();
        simultaneous_ = new WorkflowActionList(
            "Simultaneous Actions",
            "These actions start together with this Action.",
            node_, nodes_, wf->simultaneous_node_ids, wf->simultaneous_node_count, this);
        endActions_ = new WorkflowActionList(
            "End Actions",
            "These actions start after this Action completes and its End Delay has elapsed.",
            node_, nodes_, wf->end_node_ids, wf->end_node_count, this);
        nextActions_ = new WorkflowActionList(
            "Next Actions",
            "These actions are the next workflow nodes after this Action.",
            node_, nodes_, wf->next_node_ids, wf->next_node_count, this);
        layout->addWidget(simultaneous_);
        layout->addWidget(endActions_);
        layout->addWidget(nextActions_);
    }

    static QSpinBox *makeMilliseconds(QWidget *parent)
    {
        auto *spin = new QSpinBox(parent);
        spin->setRange(0, 3600000);
        spin->setSuffix(" ms");
        return spin;
    }

    static void addSpinRow(QVBoxLayout *layout, const QString &label, QSpinBox *spin, QCheckBox *defaultCheck)
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel(label));
        row->addWidget(spin, 1);
        row->addWidget(defaultCheck);
        layout->addLayout(row);
    }

    static void connectDefaultToggle(QCheckBox *check, QSpinBox *spin)
    {
        spin->setEnabled(!check->isChecked());
        QObject::connect(check, &QCheckBox::toggled, spin, &QSpinBox::setDisabled);
    }

    static bool addSourceToCombo(void *data, obs_source_t *source)
    {
        auto *combo = static_cast<QComboBox *>(data);
        if (!combo || !source)
            return true;
        const QString name = QString::fromUtf8(obs_source_get_name(source));
        if (combo->findData(name) < 0)
            combo->addItem(name, name);
        return true;
    }

    void populateSources(const QString &wanted)
    {
        configure_searchable_combo(source_);
        source_->blockSignals(true);
        source_->clear();
        obs_enum_scenes(addSourceToCombo, source_);
        obs_enum_sources(addSourceToCombo, source_);
        source_->blockSignals(false);
        const int index = source_->findData(wanted);
        if (index >= 0)
            source_->setCurrentIndex(index);
        else if (source_->count() > 0)
            source_->setCurrentIndex(0);
    }

    static void addFilterToCombo(obs_source_t *parent, obs_source_t *filter, void *param)
    {
        Q_UNUSED(parent);
        auto *combo = static_cast<QComboBox *>(param);
        if (!combo || !filter)
            return;
        const char *id = obs_source_get_id(filter);
        if (!is_supported_move_filter(id))
            return;
        const QString name = QString::fromUtf8(obs_source_get_name(filter));
        combo->addItem(name, name);
    }

    void populateFilters(const QString &wanted = QString())
    {
        filter_->blockSignals(true);
        filter_->clear();
        const QString parentName = source_->currentData().toString().isEmpty()
                                        ? source_->currentText().trimmed()
                                        : source_->currentData().toString();
        if (!parentName.isEmpty()) {
            obs_source_t *parent = obs_get_source_by_name(parentName.toUtf8().constData());
            if (parent) {
                obs_source_enum_filters(parent, addFilterToCombo, filter_);
                obs_source_release(parent);
            }
        }
        filter_->blockSignals(false);
        const int index = filter_->findData(wanted);
        if (index >= 0)
            filter_->setCurrentIndex(index);
        else if (filter_->count() > 0)
            filter_->setCurrentIndex(0);
    }

    NodeItem *node_ = nullptr;
    QList<NodeItem *> nodes_;
    QLineEdit *name_ = nullptr;
    QComboBox *triggerAction_ = nullptr;
    QGroupBox *triggerSettingsBox_ = nullptr;
    QVBoxLayout *triggerSettingsLayout_ = nullptr;
    QComboBox *triggerSource_ = nullptr;
    QComboBox *triggerFilter_ = nullptr;
    QComboBox *triggerState_ = nullptr;
    QSpinBox *triggerAudioTrack_ = nullptr;
    QSpinBox *triggerUdpPort_ = nullptr;
    QComboBox *triggerActionValue_ = nullptr;
    QLineEdit *triggerHotkey_ = nullptr;
    QLineEdit *triggerSettingName_ = nullptr;
    QLineEdit *triggerValue_ = nullptr;
    QLineEdit *triggerMatch_ = nullptr;
    QComboBox *source_ = nullptr;
    QComboBox *filter_ = nullptr;
    QSpinBox *startDelayMs_ = nullptr;
    QSpinBox *durationMs_ = nullptr;
    QSpinBox *endDelayMs_ = nullptr;
    QCheckBox *startDelayDefault_ = nullptr;
    QCheckBox *durationDefault_ = nullptr;
    QCheckBox *endDelayDefault_ = nullptr;
    WorkflowActionList *simultaneous_ = nullptr;
    WorkflowActionList *endActions_ = nullptr;
    WorkflowActionList *nextActions_ = nullptr;
    WorkflowActionList *startActions_ = nullptr;
};

} // namespace

bool edit_node_settings(NodeItem *node, const QList<NodeItem *> &nodes, QWidget *parent)
{
    NodeSettingsDialog dialog(node, nodes, parent);
    return dialog.exec() == QDialog::Accepted;
}
