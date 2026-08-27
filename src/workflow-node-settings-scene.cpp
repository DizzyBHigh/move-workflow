#include "workflow-node-settings.h"
#include "workflow-change-scene.h"
#include "workflow-node-settings-common.h"

#include <obs.h>
#include <obs-frontend-api.h>

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

namespace {
bool add_scene(void *data, obs_source_t *source)
{
    auto *combo = static_cast<QComboBox *>(data);
    if (!combo || !source)
        return true;
    const char *name = obs_source_get_name(source);
    if (name && combo->findData(QString::fromUtf8(name)) < 0)
        combo->addItem(QString::fromUtf8(name), QString::fromUtf8(name));
    return true;
}
}

void NodeSettingsDialog::buildChangeSceneEditor(QWidget *parent, QVBoxLayout *layout)
{
    auto *target = new QGroupBox("Change Scene", parent);
    auto *targetLayout = new QVBoxLayout(target);
    scene_ = new QComboBox(target);
    settings_searchable(scene_);
    obs_enum_scenes(add_scene, scene_);
    const QString wanted = QString::fromUtf8(node_->workflowNode()->action.scene_name);
    const int index = scene_->findData(wanted);
    if (index >= 0)
        scene_->setCurrentIndex(index);
    else if (scene_->count())
        scene_->setCurrentIndex(0);
    targetLayout->addWidget(new QLabel("Target Scene", target));
    targetLayout->addWidget(scene_);
    targetLayout->addWidget(new QLabel(
        QString("Default duration: %1 ms (current OBS transition)")
            .arg(workflow_change_scene_transition_duration()), target));
    layout->addWidget(target);
}
