#include "workflow-filter-instance.h"

#include "workflow-debug.h"
#include "workflow-filter-settings.h"
#include "workflow-source-lookup.h"

#include <cstdlib>
#include <cstring>

struct workflow_filter_instance_set {
    workflow_t *workflow;
    workflow_filter_instance *instances[WORKFLOW_MAX_NODES];
    char node_ids[WORKFLOW_MAX_NODES][WORKFLOW_MAX_NAME];
    size_t count;
};

static bool find_index(const workflow_filter_instance_set *set,const char *node_id,size_t *index){
    if(!set||!node_id)return false;
    for(size_t i=0;i<set->count;++i)if(!strcmp(set->node_ids[i],node_id)){if(index)*index=i;return true;}
    return false;
}

struct filter_lookup_context { const char *uuid; obs_source_t *filter; };

static void find_filter_by_uuid(obs_source_t *,obs_source_t *filter,void *data){
    auto *context=(filter_lookup_context *)data;
    if(!context||context->filter||!filter||!context->uuid)return;
    const char *uuid=obs_source_get_uuid(filter);
    if(uuid&&!strcmp(uuid,context->uuid))context->filter=obs_source_get_ref(filter);
}

static obs_source_t *find_action_filter(obs_source_t *parent,const workflow_action_ref_t *action){
    if(!parent||!action)return nullptr;
    if(action->filter_uuid[0]){
        filter_lookup_context context{action->filter_uuid,nullptr};
        obs_source_enum_filters(parent,find_filter_by_uuid,&context);
        if(context.filter)return context.filter;
    }
    if(!action->filter_name[0])return nullptr;
    return obs_source_get_filter_by_name(parent,action->filter_name);
}

workflow_filter_instance_set *workflow_filter_instance_set_create(workflow_t *workflow){
    if(!workflow||!workflow->enabled)return nullptr;
    auto *set=(workflow_filter_instance_set *)calloc(1,sizeof(workflow_filter_instance_set));
    if(!set)return nullptr;
    set->workflow=workflow;
    for(size_t i=0;i<workflow->node_count;++i){
        workflow_node_t *node=&workflow->nodes[i];
        if(node->type==WORKFLOW_NODE_ACTION&&node->action.kind!=WORKFLOW_CHANGE_SCENE&&
           !workflow_filter_instance_set_prepare_node(set,node))
            workflow_debug_log("Filter instance: node='%s' has no valid Move filter; skipping preparation",node->id);
    }
    workflow_debug_log("Filter instances: prepared %zu runtime Move filters",set->count);
    return set;
}

bool workflow_filter_instance_set_prepare_node(workflow_filter_instance_set *set,workflow_node_t *node){
    if(!set||!node||node->type!=WORKFLOW_NODE_ACTION||node->action.kind==WORKFLOW_CHANGE_SCENE)return false;
    if(find_index(set,node->id,nullptr)||set->count>=WORKFLOW_MAX_NODES)return find_index(set,node->id,nullptr);
    obs_source_t *parent=nullptr;
    if(node->action.source_uuid[0])parent=workflow_find_source_by_uuid(node->action.source_uuid);
    if(!parent&&node->action.source_name[0])parent=obs_get_source_by_name(node->action.source_name);
    if(!parent&&node->action.scene_name[0])parent=obs_get_source_by_name(node->action.scene_name);
    if(!parent){
        workflow_debug_log("Filter instance: node='%s' parent UUID='%s' name='%s' not found",node->id,node->action.source_uuid,node->action.source_name[0]?node->action.source_name:node->action.scene_name);
        return false;
    }
    obs_source_t *original=find_action_filter(parent,&node->action);
    if(!original){obs_source_release(parent);return false;}
    const char *expected=workflow_expected_filter_id(node->action.kind);
    const char *actual=obs_source_get_id(original);
    if(!expected||!actual||strcmp(expected,actual)){obs_source_release(original);obs_source_release(parent);return false;}
    if(!node->action.source_uuid[0]){
        const char *uuid=obs_source_get_uuid(parent);
        if(uuid&&*uuid)strncpy(node->action.source_uuid,uuid,WORKFLOW_MAX_NAME-1);
    }
    if(!node->action.filter_uuid[0]){
        const char *uuid=obs_source_get_uuid(original);
        if(uuid&&*uuid)strncpy(node->action.filter_uuid,uuid,WORKFLOW_MAX_NAME-1);
    }
    workflow_filter_instance *instance=workflow_filter_instance_create(original,parent,node);
    obs_source_release(original);obs_source_release(parent);
    if(!instance)return false;
    uint64_t duration=0,restore_delay=0;
    workflow_filter_apply_node_settings(instance->instance,node,&duration,&restore_delay);
    set->instances[set->count]=instance;
    strncpy(set->node_ids[set->count],node->id,WORKFLOW_MAX_NAME-1);
    set->node_ids[set->count][WORKFLOW_MAX_NAME-1]='\0';
    ++set->count;
    workflow_debug_log("Filter instance: node='%s' prepared runtime='%s' uuid='%s' duration=%llu",node->id,obs_source_get_name(instance->instance),node->action.filter_uuid,(unsigned long long)duration);
    return true;
}

workflow_filter_instance *workflow_filter_instance_set_get(workflow_filter_instance_set *set,const workflow_node_t *node){size_t index=0;return node&&find_index(set,node->id,&index)?set->instances[index]:nullptr;}

void workflow_filter_instance_set_destroy(workflow_filter_instance_set *set){
    if(!set)return;
    for(size_t i=0;i<set->count;++i)workflow_filter_instance_destroy(set->instances[i]);
    free(set);
}