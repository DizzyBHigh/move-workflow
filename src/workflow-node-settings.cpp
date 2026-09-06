#include "workflow-node-settings.h"
#include "workflow-action-list.h"
#include "workflow-node-settings-common.h"
#include <obs.h>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

NodeSettingsDialog::NodeSettingsDialog(NodeItem *node,const QList<NodeItem *> &nodes,QWidget *parent):QDialog(parent),node_(node),nodes_(nodes)
{
    setWindowTitle(QString("Node Settings - %1").arg(node?node->nodeName():"Node")); resize(560,800); setMinimumSize(500,400);
    auto *root=new QVBoxLayout(this); auto *area=new QScrollArea(this); area->setWidgetResizable(true); area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *content=new QWidget; auto *layout=new QVBoxLayout(content); layout->setSpacing(8); const bool trigger=node&&node->workflowNode()->type==WORKFLOW_NODE_TRIGGER;
    auto *box=new QGroupBox(trigger?"Trigger Node":"Node",content); auto *boxLayout=new QVBoxLayout(box); name_=new QLineEdit(node?node->nodeName():QString(),box); boxLayout->addWidget(new QLabel("Name",box)); boxLayout->addWidget(name_); layout->addWidget(box);
    if(trigger) buildTriggerEditor(boxLayout,layout); else buildActionEditor(content,layout); layout->addStretch(1); area->setWidget(content); root->addWidget(area,1);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); connect(buttons,&QDialogButtonBox::accepted,this,[this]{if(apply())accept();}); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject); root->addWidget(buttons);
}

bool NodeSettingsDialog::apply()
{
    if(!node_)return false; const QString name=name_->text().trimmed(); if(name.isEmpty())return false; auto *wf=node_->workflowNode(); settings_copy_text(wf->name,WORKFLOW_MAX_NAME,name); if(wf->type==WORKFLOW_NODE_TRIGGER)return applyTrigger();
    const bool changeScene=actionType_&&actionType_->currentData().toString()=="scene";
    if(changeScene){
        const QString sceneName=scene_?scene_->currentText().trimmed():QString();
        if(sceneName.isEmpty()){
            wf->action.scene_name[0]='\0'; wf->action.source_uuid[0]='\0'; wf->action.source_name[0]='\0'; wf->action.filter_name[0]='\0'; wf->action.filter_id[0]='\0'; wf->action.filter_uuid[0]='\0';
        } else {
            obs_source_t *scene=obs_get_source_by_name(sceneName.toUtf8().constData()); if(!scene)return false; const char *uuid=obs_source_get_uuid(scene); if(!uuid||!*uuid){obs_source_release(scene);return false;}
            wf->action.kind=WORKFLOW_CHANGE_SCENE; settings_copy_text(wf->action.scene_name,WORKFLOW_MAX_NAME,sceneName); settings_copy_text(wf->action.source_uuid,WORKFLOW_MAX_NAME,QString::fromUtf8(uuid)); settings_copy_text(wf->action.source_name,WORKFLOW_MAX_NAME,sceneName); wf->action.filter_name[0]='\0'; wf->action.filter_id[0]='\0'; wf->action.filter_uuid[0]='\0'; obs_source_release(scene);
        }
    } else {
        const QString parentName=source_->currentText().trimmed();
        const QString filterName=filter_->currentText().trimmed();
        const QString parentUuid=source_->currentData().toString();
        const QString filterUuid=filter_->currentData().toString();
        obs_source_t *parent=!parentUuid.isEmpty()?obs_get_source_by_uuid(parentUuid.toUtf8().constData()):obs_get_source_by_name(parentName.toUtf8().constData());
        if(parentName.isEmpty()){
            wf->action.kind=WORKFLOW_MOVE_ACTION; wf->action.scene_name[0]='\0'; wf->action.source_uuid[0]='\0'; wf->action.source_name[0]='\0'; wf->action.filter_name[0]='\0'; wf->action.filter_id[0]='\0'; wf->action.filter_uuid[0]='\0';
        } else if(filterName.isEmpty()){
            if(!parent)return false; const char *uuid=obs_source_get_uuid(parent); if(!uuid||!*uuid){obs_source_release(parent);return false;}
            wf->action.kind=WORKFLOW_MOVE_ACTION; settings_copy_text(wf->action.scene_name,WORKFLOW_MAX_NAME,parentName); settings_copy_text(wf->action.source_uuid,WORKFLOW_MAX_NAME,QString::fromUtf8(uuid)); settings_copy_text(wf->action.source_name,WORKFLOW_MAX_NAME,parentName); wf->action.filter_name[0]='\0'; wf->action.filter_id[0]='\0'; wf->action.filter_uuid[0]='\0'; obs_source_release(parent);
        } else {
            if(!parent)return false;
            obs_source_t *filter=nullptr;
            if(!filterUuid.isEmpty()){
                struct Context{QString uuid;obs_source_t *filter;}; Context ctx{filterUuid,nullptr};
                obs_source_enum_filters(parent,[](obs_source_t *,obs_source_t *candidate,void *data){auto *c=static_cast<Context*>(data);if(!c->filter&&QString::fromUtf8(obs_source_get_uuid(candidate))==c->uuid)c->filter=obs_source_get_ref(candidate);},&ctx);
                filter=ctx.filter;
            }
            if(!filter)filter=obs_source_get_filter_by_name(parent,filterName.toUtf8().constData());
            if(!filter){obs_source_release(parent);return false;}
            const char *filterId=obs_source_get_id(filter); const char *resolvedFilterUuid=obs_source_get_uuid(filter); const char *resolvedParentUuid=obs_source_get_uuid(parent); if(!settings_supported_filter(filterId)||!resolvedFilterUuid||!*resolvedFilterUuid||!resolvedParentUuid||!*resolvedParentUuid){obs_source_release(filter);obs_source_release(parent);return false;}
            wf->action.kind=settings_kind(filterId); settings_copy_text(wf->action.scene_name,WORKFLOW_MAX_NAME,parentName); settings_copy_text(wf->action.source_uuid,WORKFLOW_MAX_NAME,QString::fromUtf8(resolvedParentUuid)); settings_copy_text(wf->action.source_name,WORKFLOW_MAX_NAME,parentName); settings_copy_text(wf->action.filter_name,WORKFLOW_MAX_NAME,QString::fromUtf8(obs_source_get_name(filter))); settings_copy_text(wf->action.filter_id,WORKFLOW_MAX_NAME,QString::fromUtf8(filterId)); settings_copy_text(wf->action.filter_uuid,WORKFLOW_MAX_NAME,QString::fromUtf8(resolvedFilterUuid)); obs_source_release(filter); obs_source_release(parent);
        }
    }
    wf->start_delay.mode=startDelayDefault_->isChecked()?WORKFLOW_USE_EXISTING:WORKFLOW_OVERRIDE; wf->start_delay.delay_ms=startDelayDefault_->isChecked()?startDelayOverrideMs_:(uint64_t)startDelayMs_->value();
    wf->duration.mode=durationDefault_->isChecked()?WORKFLOW_USE_EXISTING:WORKFLOW_OVERRIDE; wf->duration.duration_ms=durationDefault_->isChecked()?durationOverrideMs_:(uint64_t)durationMs_->value();
    wf->end_delay.mode=endDelayDefault_->isChecked()?WORKFLOW_USE_EXISTING:WORKFLOW_OVERRIDE; wf->end_delay.delay_ms=endDelayDefault_->isChecked()?endDelayOverrideMs_:(uint64_t)endDelayMs_->value();
    wf->easing.mode=easingDefault_->isChecked()?WORKFLOW_USE_EXISTING:WORKFLOW_OVERRIDE; wf->easing.easing=(workflow_easing_t)easingType_->currentData().toInt(); wf->easing.function=(workflow_easing_function_t)easingFunction_->currentData().toInt();
    simultaneous_->apply(wf->simultaneous_node_count,wf->simultaneous_node_ids); nextActions_->apply(wf->next_node_count,wf->next_node_ids); shortcutActions_->apply(wf->shortcut_node_count,wf->shortcut_node_ids); wf->simultaneous_actions_mode=WORKFLOW_OVERRIDE; wf->next_actions_mode=WORKFLOW_OVERRIDE; return true;
}

bool edit_node_settings(NodeItem *node,const QList<NodeItem *> &nodes,QWidget *parent){NodeSettingsDialog dialog(node,nodes,parent);return dialog.exec()==QDialog::Accepted;}
