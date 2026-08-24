#pragma once

#include "workflow-node.h"

#include <QList>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class QVBoxLayout;
class QWidget;
class WorkflowActionList;

class NodeSettingsDialog final : public QDialog {
public:
    NodeSettingsDialog(NodeItem *node, const QList<NodeItem *> &nodes,
                       QWidget *parent = nullptr);

private:
    bool apply();
    bool applyTrigger();
    void buildTriggerEditor(QVBoxLayout *layout, QVBoxLayout *contentLayout);
    void buildActionEditor(QWidget *parent, QVBoxLayout *layout);

    void rebuildTriggerSettings();
    void clearTriggerSettings();
    void addTriggerRow(const QString &label, QWidget *widget);
    void buildSourceStateSettings(const QString &label, workflow_trigger_state_t state);
    void buildSourceAudioTrackSettings(const workflow_trigger_ref_t &trigger);
    void buildSourceHotkeySettings(const workflow_trigger_ref_t &trigger);
    void buildFilterEnableSettings(const workflow_trigger_ref_t &trigger);
    void buildSettingSettings(const workflow_trigger_ref_t &trigger);
    void populateTriggerSources(const QString &wanted = QString());
    void populateTriggerFilters(const QString &wanted = QString());

    void populateSources(const QString &wanted);
    void populateFilters(const QString &wanted = QString());

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

bool edit_node_settings(NodeItem *node, const QList<NodeItem *> &nodes,
                        QWidget *parent = nullptr);
