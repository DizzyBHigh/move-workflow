#pragma once

#include "workflow-node.h"
#include "workflow-trigger-filter-instance.h"
#include <QDialog>
#include <QList>
#include <QString>
#include <QVector>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QVBoxLayout;
class QWidget;
class WorkflowActionList;

class NodeSettingsDialog final : public QDialog {
public:
    NodeSettingsDialog(NodeItem *node, const QList<NodeItem *> &nodes, QWidget *parent = nullptr);
private:
    struct TriggerRow { QComboBox *source=nullptr; QComboBox *trigger=nullptr; QPushButton *remove=nullptr; };
    struct TriggerSelection { QString sourceUuid; QString filterUuid; };
    bool apply();
    bool applyTrigger();
    void buildTriggerEditor(QVBoxLayout *, QVBoxLayout *);
    void buildActionEditor(QWidget *, QVBoxLayout *);
    void buildChangeSceneEditor(QWidget *, QVBoxLayout *);
    void rebuildTriggerRows();
    void addTriggerRow(const TriggerSelection &selection = {});
    void populateTriggerSources(QComboBox *, const QString & = QString());
    void populateTriggerFilters(QComboBox *, const QString &, const QString & = QString());
    void buildActionSettings(const workflow_action_ref_t &);
    void populateSources(const QString &);
    void populateFilters(const QString & = QString());

    NodeItem *node_=nullptr;
    QList<NodeItem *> nodes_;
    QLineEdit *name_=nullptr;
    QGroupBox *triggerBox_=nullptr;
    QVBoxLayout *triggerRowsLayout_=nullptr;
    QVector<TriggerRow> triggerRows_;
    QComboBox *actionType_=nullptr;
    QStackedWidget *actionTargetStack_=nullptr;
    QComboBox *scene_=nullptr;
    QComboBox *source_=nullptr;
    QComboBox *filter_=nullptr;
    QSpinBox *startDelayMs_=nullptr,*durationMs_=nullptr,*endDelayMs_=nullptr;
    QCheckBox *startDelayDefault_=nullptr,*durationDefault_=nullptr,*endDelayDefault_=nullptr;
    uint64_t startDelayOverrideMs_=0,durationOverrideMs_=0,endDelayOverrideMs_=0;
    WorkflowActionList *simultaneous_=nullptr,*nextActions_=nullptr,*shortcutActions_=nullptr,*startActions_=nullptr;
};

bool edit_node_settings(NodeItem *node, const QList<NodeItem *> &nodes, QWidget *parent=nullptr);
