#include "workflow-node-settings.h"
#include "workflow-action-list.h"
#include "workflow-node-settings-common.h"
#include "workflow-node-timing-defaults.h"
#include "workflow-change-scene.h"
#include <obs.h>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {
static QSpinBox *milliseconds(QWidget *parent) { auto *s=new QSpinBox(parent); s->setRange(0,3600000); s->setSuffix(" ms"); return s; }
static void spin_row(QVBoxLayout *l,const QString &label,QSpinBox *s,QCheckBox *c){auto *r=new QHBoxLayout;r->addWidget(new QLabel(label));r->addWidget(s,1);r->addWidget(c);l->addLayout(r);}
static bool add_source(void *data,obs_source_t *source){auto *c=static_cast<QComboBox*>(data);if(!c||!source)return true;QString n=QString::fromUtf8(obs_source_get_name(source));if(c->findData(n)<0)c->addItem(n,n);return true;}
static void add_filter(obs_source_t *,obs_source_t *filter,void *data){auto *c=static_cast<QComboBox*>(data);if(!c||!filter||!settings_supported_filter(obs_source_get_id(filter)))return;c->addItem(QString::fromUtf8(obs_source_get_name(filter)),QString::fromUtf8(obs_source_get_name(filter)));}
static bool add_scene(void *data,obs_source_t *source){auto *c=static_cast<QComboBox*>(data);if(!c||!source)return true;QString n=QString::fromUtf8(obs_source_get_name(source));if(c->findData(n)<0)c->addItem(n,n);return true;}
}

void NodeSettingsDialog::buildActionEditor(QWidget *parent,QVBoxLayout *layout)
{
    const workflow_node_t *wf=node_->workflowNode();
    auto *typeBox=new QGroupBox("Action Type",parent); auto *typeLayout=new QVBoxLayout(typeBox);
    actionType_=new QComboBox(typeBox); actionType_->addItem("Move","move"); actionType_->addItem("Change Scene","scene");
    actionType_->setCurrentIndex(wf->action.kind==WORKFLOW_CHANGE_SCENE?1:0); typeLayout->addWidget(actionType_); layout->addWidget(typeBox);
    actionTargetStack_=new QStackedWidget(parent);
    auto *moveTarget=new QGroupBox("Move",parent); auto *moveLayout=new QVBoxLayout(moveTarget);
    source_=new QComboBox(moveTarget); filter_=new QComboBox(moveTarget); settings_searchable(source_);
    moveLayout->addWidget(new QLabel("Source",moveTarget)); moveLayout->addWidget(source_); moveLayout->addWidget(new QLabel("Filter",moveTarget)); moveLayout->addWidget(filter_);
    populateSources(QString::fromUtf8(wf->action.scene_name)); populateFilters(QString::fromUtf8(wf->action.filter_name));
    auto *sceneTarget=new QGroupBox("Change Scene",parent); auto *sceneLayout=new QVBoxLayout(sceneTarget);
    scene_=new QComboBox(sceneTarget); obs_enum_scenes(add_scene,scene_); QString wanted=QString::fromUtf8(wf->action.scene_name); int si=scene_->findData(wanted); if(si>=0)scene_->setCurrentIndex(si); else if(scene_->count())scene_->setCurrentIndex(0);
    sceneLayout->addWidget(new QLabel("Target Scene",sceneTarget)); sceneLayout->addWidget(scene_); sceneLayout->addWidget(new QLabel(QString("Default duration: %1 ms (current OBS transition)").arg(workflow_change_scene_transition_duration()),sceneTarget));
    actionTargetStack_->addWidget(moveTarget); actionTargetStack_->addWidget(sceneTarget); actionTargetStack_->setCurrentIndex(actionType_->currentIndex()); layout->addWidget(actionTargetStack_);
    connect(actionType_,&QComboBox::currentIndexChanged,actionTargetStack_,&QStackedWidget::setCurrentIndex); connect(source_,&QComboBox::currentIndexChanged,this,[this]{populateFilters();});
    auto *timing=new QGroupBox("Timing",parent); auto *timingLayout=new QVBoxLayout(timing);
    startDelayMs_=milliseconds(timing); durationMs_=milliseconds(timing); endDelayMs_=milliseconds(timing);
    startDelayDefault_=new QCheckBox("Use default",timing); durationDefault_=new QCheckBox("Use default",timing); endDelayDefault_=new QCheckBox("Use default",timing);
    startDelayOverrideMs_=wf->start_delay.delay_ms; durationOverrideMs_=wf->duration.duration_ms; endDelayOverrideMs_=wf->end_delay.delay_ms;
    startDelayMs_->setValue((int)startDelayOverrideMs_); durationMs_->setValue((int)durationOverrideMs_); endDelayMs_->setValue((int)endDelayOverrideMs_);
    startDelayDefault_->setChecked(wf->start_delay.mode==WORKFLOW_USE_EXISTING); durationDefault_->setChecked(wf->duration.mode==WORKFLOW_USE_EXISTING); endDelayDefault_->setChecked(wf->end_delay.mode==WORKFLOW_USE_EXISTING);
    spin_row(timingLayout,"Start Delay",startDelayMs_,startDelayDefault_); spin_row(timingLayout,"Duration",durationMs_,durationDefault_); spin_row(timingLayout,"End Delay",endDelayMs_,endDelayDefault_); layout->addWidget(timing);
    auto *easing=new QGroupBox("Easing",parent); auto *easingLayout=new QVBoxLayout(easing); easingDefault_=new QCheckBox("Use default",easing); easingType_=new QComboBox(easing); easingFunction_=new QComboBox(easing); easingType_->addItem("No Easing",WORKFLOW_EASE_NONE); easingType_->addItem("Ease In",WORKFLOW_EASE_IN); easingType_->addItem("Ease Out",WORKFLOW_EASE_OUT); easingType_->addItem("Ease In/Out",WORKFLOW_EASE_IN_OUT); easingFunction_->addItem("Quadratic",WORKFLOW_EASING_QUADRATIC); easingFunction_->addItem("Cubic",WORKFLOW_EASING_CUBIC); easingFunction_->addItem("Quartic",WORKFLOW_EASING_QUARTIC); easingFunction_->addItem("Quintic",WORKFLOW_EASING_QUINTIC); easingFunction_->addItem("Sine",WORKFLOW_EASING_SINE); easingFunction_->addItem("Circular",WORKFLOW_EASING_CIRCULAR); easingFunction_->addItem("Exponential",WORKFLOW_EASING_EXPONENTIAL); easingFunction_->addItem("Elastic",WORKFLOW_EASING_ELASTIC); easingFunction_->addItem("Bounce",WORKFLOW_EASING_BOUNCE); easingFunction_->addItem("Back",WORKFLOW_EASING_BACK); easingLayout->addWidget(new QLabel("Easing",easing)); easingLayout->addWidget(easingType_); easingLayout->addWidget(new QLabel("Easing Function",easing)); easingLayout->addWidget(easingFunction_); easingLayout->addWidget(easingDefault_); layout->addWidget(easing);
    auto setOriginalEasing=[this]{QString sourceName=source_->currentData().toString(),filterName=filter_->currentData().toString();if(sourceName.isEmpty()||filterName.isEmpty())return;obs_source_t *parent=obs_get_source_by_name(sourceName.toUtf8().constData());if(!parent)return;struct Context{QString name;obs_source_t *filter;};Context ctx{filterName,nullptr};obs_source_enum_filters(parent,[](obs_source_t *,obs_source_t *candidate,void *data){auto *c=static_cast<Context*>(data);if(!c->filter&&QString::fromUtf8(obs_source_get_name(candidate))==c->name)c->filter=obs_source_get_ref(candidate);},&ctx);if(ctx.filter){obs_data_t *settings=obs_source_get_settings(ctx.filter);int easing=obs_data_get_int(settings,"easing_match"),function=obs_data_get_int(settings,"easing_function_match");int ei=easingType_->findData(easing),efi=easingFunction_->findData(function);if(ei>=0)easingType_->setCurrentIndex(ei);if(efi>=0)easingFunction_->setCurrentIndex(efi);obs_data_release(settings);obs_source_release(ctx.filter);}obs_source_release(parent);};
    if(wf->easing.mode==WORKFLOW_USE_EXISTING)setOriginalEasing(); else {int ei=easingType_->findData(wf->easing.easing);if(ei>=0)easingType_->setCurrentIndex(ei);int efi=easingFunction_->findData(wf->easing.function);if(efi>=0)easingFunction_->setCurrentIndex(efi);}
    easingDefault_->setChecked(wf->easing.mode==WORKFLOW_USE_EXISTING); easingType_->setEnabled(!easingDefault_->isChecked()); easingFunction_->setEnabled(!easingDefault_->isChecked());
    connect(easingDefault_,&QCheckBox::toggled,this,[this,setOriginalEasing](bool checked){if(!checked)setOriginalEasing();easingType_->setEnabled(!checked);easingFunction_->setEnabled(!checked);});
    auto refreshDefaults=[this]{ if(actionType_->currentData().toString()=="scene"){if(startDelayDefault_->isChecked())startDelayMs_->setValue(0);if(durationDefault_->isChecked())durationMs_->setValue((int)workflow_change_scene_transition_duration());if(endDelayDefault_->isChecked())endDelayMs_->setValue(0);return;} const auto d=workflow_node_read_timing_defaults(source_->currentData().toString().toUtf8().constData(),filter_->currentData().toString().toUtf8().constData());if(!d.valid)return;if(startDelayDefault_->isChecked())startDelayMs_->setValue((int)d.start_delay_ms);if(durationDefault_->isChecked())durationMs_->setValue((int)d.duration_ms);if(endDelayDefault_->isChecked())endDelayMs_->setValue((int)d.end_delay_ms);};
    auto toggle=[this,refreshDefaults](QCheckBox *c,QSpinBox *s,uint64_t &v){if(c->isChecked()){v=(uint64_t)s->value();refreshDefaults();s->setEnabled(false);}else{s->setEnabled(true);s->setValue((int)v);}};
    connect(startDelayDefault_,&QCheckBox::toggled,this,[this,toggle]{toggle(startDelayDefault_,startDelayMs_,startDelayOverrideMs_);}); connect(durationDefault_,&QCheckBox::toggled,this,[this,toggle]{toggle(durationDefault_,durationMs_,durationOverrideMs_);}); connect(endDelayDefault_,&QCheckBox::toggled,this,[this,toggle]{toggle(endDelayDefault_,endDelayMs_,endDelayOverrideMs_);});
    connect(startDelayMs_,&QSpinBox::valueChanged,this,[this](int v){if(!startDelayDefault_->isChecked())startDelayOverrideMs_=(uint64_t)v;}); connect(durationMs_,&QSpinBox::valueChanged,this,[this](int v){if(!durationDefault_->isChecked())durationOverrideMs_=(uint64_t)v;}); connect(endDelayMs_,&QSpinBox::valueChanged,this,[this](int v){if(!endDelayDefault_->isChecked())endDelayOverrideMs_=(uint64_t)v;});
    connect(filter_,&QComboBox::currentIndexChanged,this,[this,refreshDefaults,setOriginalEasing]{refreshDefaults();if(easingDefault_->isChecked())setOriginalEasing();}); connect(source_,&QComboBox::currentIndexChanged,this,[this,refreshDefaults,setOriginalEasing]{populateFilters();refreshDefaults();if(easingDefault_->isChecked())setOriginalEasing();}); connect(actionType_,&QComboBox::currentIndexChanged,this,[refreshDefaults]{refreshDefaults();});
    refreshDefaults(); startDelayMs_->setEnabled(!startDelayDefault_->isChecked()); durationMs_->setEnabled(!durationDefault_->isChecked()); endDelayMs_->setEnabled(!endDelayDefault_->isChecked());
    simultaneous_=new WorkflowActionList("Simultaneous Actions","These actions start together with this Action.",node_,nodes_,wf->simultaneous_node_ids,wf->simultaneous_node_count,this); nextActions_=new WorkflowActionList("Next Actions","These actions start after this Action's duration and End Delay.",node_,nodes_,wf->next_node_ids,wf->next_node_count,this); shortcutActions_=new WorkflowActionList("Shortcut Actions","These actions wait for their configured OBS shortcut.",node_,nodes_,wf->shortcut_node_ids,wf->shortcut_node_count,this); layout->addWidget(simultaneous_);layout->addWidget(nextActions_);layout->addWidget(shortcutActions_);
}

void NodeSettingsDialog::populateSources(const QString &wanted){settings_searchable(source_);source_->blockSignals(true);source_->clear();obs_enum_scenes(add_source,source_);obs_enum_sources(add_source,source_);source_->blockSignals(false);int i=source_->findData(wanted);if(i>=0)source_->setCurrentIndex(i);else if(source_->count())source_->setCurrentIndex(0);}
void NodeSettingsDialog::populateFilters(const QString &wanted){filter_->blockSignals(true);filter_->clear();QString n=source_->currentData().toString().isEmpty()?source_->currentText().trimmed():source_->currentData().toString();obs_source_t *p=n.isEmpty()?nullptr:obs_get_source_by_name(n.toUtf8().constData());if(p){obs_source_enum_filters(p,add_filter,filter_);obs_source_release(p);}filter_->blockSignals(false);int i=filter_->findData(wanted);if(i>=0)filter_->setCurrentIndex(i);else if(filter_->count())filter_->setCurrentIndex(0);}
