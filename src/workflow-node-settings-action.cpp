#include "workflow-node-settings.h"

#include "workflow-action-list.h"
#include "workflow-node-settings-common.h"

#include <obs.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
static QSpinBox *milliseconds(QWidget *parent)
{
    auto *spin = new QSpinBox(parent);
    spin->setRange(0, 3600000);
    spin->setSuffix(" ms");
    return spin;
}

static void spin_row(QVBoxLayout *layout, const QString &label,
                     QSpinBox *spin, QCheckBox *check)
{
    auto *row = new QHBoxLayout;
    row->addWidget(new QLabel(label));
    row->addWidget(spin, 1);
    row->addWidget(check);
    layout->addLayout(row);
}

static bool add_source(void *data, obs_source_t *source)
{
    auto *combo = static_cast<QComboBox *>(data);
    if (!combo || !source)
        return true;
    const QString name = QString::fromUtf8(obs_source_get_name(source));
    if (combo->findData(name) < 0)
        combo->addItem(name, name);
    return true;
}

static void add_filter(obs_source_t *, obs_source_t *filter, void *data)
{
    auto *combo = static_cast<QComboBox *>(data);
    if (!combo || !filter || !settings_supported_filter(obs_source_get_id(filter)))
        return;
    const QString name = QString::fromUtf8(obs_source_get_name(filter));
    combo->addItem(name, name);
}
} // namespace

void NodeSettingsDialog::buildActionEditor(QWidget *parent, QVBoxLayout *layout)
{
    auto *target = new QGroupBox("Existing Move / Swap / Value Action", parent);
    auto *targetLayout = new QVBoxLayout(target);
    source_ = new QComboBox(target);
    filter_ = new QComboBox(target);
    targetLayout->addWidget(new QLabel("Source", target));
    targetLayout->addWidget(source_);
    targetLayout->addWidget(new QLabel("Filter", target));
    targetLayout->addWidget(filter_);
    layout->addWidget(target);

    settings_searchable(source_);
    populateSources(settings_read_text(node_->workflowNode()->action.scene_name));
    connect(source_, &QComboBox::currentIndexChanged, this, [this] { populateFilters(); });
    populateFilters(settings_read_text(node_->workflowNode()->action.filter_name));

    auto *timing = new QGroupBox("Timing", parent);
    auto *timingLayout = new QVBoxLayout(timing);
    startDelayMs_ = milliseconds(timing);
    durationMs_ = milliseconds(timing);
    endDelayMs_ = milliseconds(timing);
    startDelayDefault_ = new QCheckBox("Use default", timing);
    durationDefault_ = new QCheckBox("Use default", timing);
    endDelayDefault_ = new QCheckBox("Use default", timing);
    const workflow_node_t *wf = node_->workflowNode();
    startDelayMs_->setValue((int)wf->start_delay.delay_ms);
    durationMs_->setValue((int)wf->duration.duration_ms);
    endDelayMs_->setValue((int)wf->end_delay.delay_ms);
    startDelayDefault_->setChecked(wf->start_delay.mode == WORKFLOW_USE_EXISTING);
    durationDefault_->setChecked(wf->duration.mode == WORKFLOW_USE_EXISTING);
    endDelayDefault_->setChecked(wf->end_delay.mode == WORKFLOW_USE_EXISTING);
    spin_row(timingLayout, "Start Delay", startDelayMs_, startDelayDefault_);
    spin_row(timingLayout, "Duration", durationMs_, durationDefault_);
    spin_row(timingLayout, "End Delay", endDelayMs_, endDelayDefault_);
    layout->addWidget(timing);

    simultaneous_ = new WorkflowActionList("Simultaneous Actions",
        "These actions start together with this Action.", node_, nodes_,
        wf->simultaneous_node_ids, wf->simultaneous_node_count, this);
    endActions_ = new WorkflowActionList("End Actions",
        "These actions start after this Action completes and its End Delay has elapsed.",
        node_, nodes_, wf->end_node_ids, wf->end_node_count, this);
    nextActions_ = new WorkflowActionList("Next Actions",
        "These actions are the next workflow nodes after this Action.",
        node_, nodes_, wf->next_node_ids, wf->next_node_count, this);
    layout->addWidget(simultaneous_);
    layout->addWidget(endActions_);
    layout->addWidget(nextActions_);
}

void NodeSettingsDialog::populateSources(const QString &wanted)
{
    settings_searchable(source_);
    source_->blockSignals(true);
    source_->clear();
    obs_enum_scenes(add_source, source_);
    obs_enum_sources(add_source, source_);
    source_->blockSignals(false);
    const int index = source_->findData(wanted);
    if (index >= 0)
        source_->setCurrentIndex(index);
    else if (source_->count())
        source_->setCurrentIndex(0);
}

void NodeSettingsDialog::populateFilters(const QString &wanted)
{
    filter_->blockSignals(true);
    filter_->clear();
    const QString parentName = source_->currentData().toString().isEmpty()
                                   ? source_->currentText().trimmed()
                                   : source_->currentData().toString();
    obs_source_t *parent = parentName.isEmpty()
                               ? nullptr
                               : obs_get_source_by_name(parentName.toUtf8().constData());
    if (parent) {
        obs_source_enum_filters(parent, add_filter, filter_);
        obs_source_release(parent);
    }
    filter_->blockSignals(false);
    const int index = filter_->findData(wanted);
    if (index >= 0)
        filter_->setCurrentIndex(index);
    else if (filter_->count())
        filter_->setCurrentIndex(0);
}
