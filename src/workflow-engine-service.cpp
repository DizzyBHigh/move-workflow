#include "workflow-engine-service.h"
#include "workflow-debug.h"
#include "workflow-engine.h"
#include "workflow-engine-node.h"
#include "workflow-persistence.h"
#include <obs-module.h>
#include <cstring>

static workflow_engine_t *service_engine;
void workflow_engine_service_set(workflow_engine_t *engine) { service_engine=engine; blog(LOG_INFO,"[Move Workflow] Engine service %s.",engine?"connected":"disconnected"); }

static workflow_t *find_workflow(const char *id)
{
    auto *m=workflow_persistence_manager();
    return m&&id?workflow_manager_find(m,id):nullptr;
}

bool workflow_engine_service_trigger(const char *workflow_id,const char *trigger_id)
{
    if(!service_engine||!workflow_id||!trigger_id)return false;
    auto *w=find_workflow(workflow_id); if(!w)return false;
    auto *n=workflow_engine_find_node(w,trigger_id);
    if(!n||n->type!=WORKFLOW_NODE_TRIGGER)return false;
    workflow_debug_log("External trigger: workflow='%s' node='%s'",w->name,n->id);
    return workflow_engine_start_trigger(service_engine,w,n->id);
}

bool workflow_engine_service_trigger_scene(const char *scene_name)
{
    if(!service_engine||!scene_name)return false;
    bool started=false; auto *m=workflow_persistence_manager(); if(!m)return false;
    for(size_t i=0;i<m->workflow_count;++i){auto *w=&m->workflows[i]; if(!w->enabled)continue; for(size_t n=0;n<w->node_count;++n){auto *node=&w->nodes[n]; if(node->type!=WORKFLOW_NODE_TRIGGER||node->trigger.type!=WORKFLOW_TRIGGER_SCENE_CHANGE)continue; if(strcmp(node->trigger.scene_name,scene_name)!=0)continue; started|=workflow_engine_start_trigger(service_engine,w,node->id);}}
    return started;
}

bool workflow_engine_service_test_node(const char *workflow_id,const char *node_id)
{
    if(!service_engine||!workflow_id||!node_id)return false;
    auto *w=find_workflow(workflow_id); if(!w||!w->enabled)return false;
    return workflow_engine_test_node(service_engine,w,node_id);
}
