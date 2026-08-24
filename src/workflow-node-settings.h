#pragma once

#include "workflow-node.h"

#include <QDialog>
#include <QList>
#include <QString>

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
    void buildTriggerEditor(QVBoxLayout *, QVBoxLayout *);
    void buildActionEditor(QWidget *, QVBoxLayout *);
    void rebuildTriggerSettings();
    void clearTriggerSettings();
    void addTriggerRow(const QString &, QWidget *);
    void buildSourceStateSettings(const QString &, workflow_trigger_state_t);
    void buildSourceAudioTrackSettings(const workflow_trigger_ref_t &);
    void buildSourceHotkeySettings(const workflow_trigger_ref_t &);
    void buildFilterEnableSettings(const workflow_trigger_ref_t &);
    void buildSettingSettings(const workflow_trigger_ref_t &);
    void populateTriggerSources(const QString & = QString());
    void populateTriggerFilters(const QString & = QString());
    void populateSources(const QString &);
    void populateFilters(const QString & = QString());

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
    WorkflowActionList *nextActions_ = nullptr;
    WorkflowActionList *shortcutActions_ = nullptr;
    WorkflowActionList *startActions_ = nullptr;
};

bool edit_node_settings(NodeItem *node, const QList<NodeItem *> &nodes,
                        QWidget *parent = nullptr);
