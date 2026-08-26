#include "workflow-action-runtime.h"
#include "workflow-change-scene.h"
#include "workflow-debug.h"
#include <cstdlib>
struct workflow_action_runtime { workflow_t *workflow; workflow_node_t *node; workflow_action_runtime_state_t state; uint64_t generation; uint64_t duration_ms; uint64_t start_delay_ms; uint64_t end_delay_ms; };
static uint64_t override_value(workflow_value_mode_t mode,uint64_t value){return mode==WORKFLOW_OVERRIDE?value:0;}
workflow_action_runtime_t *workflow_action_runtime_create(workflow_t*w,workflow_node_t*n,uint64_t generation){if(!w||!n||n->type!=WORKFLOW_NODE_ACTION)return nullptr;auto*r=(workflow_action_runtime_t*)calloc(1,sizeof(*r));if(!r)return nullptr;r->workflow=w;r->node=n;r->state=WORKFLOW_ACTION_RUNTIME_WAIT_START_DELAY;r->generation=generation;r->duration_ms=override_value(n->duration.mode,n->duration.duration_ms);if(n->action.kind==WORKFLOW_CHANGE_SCENE&&n->action.scene_completion==WORKFLOW_SCENE_COMPLETE_TRANSITION)r->duration_ms=workflow_change_scene_transition_duration();r->start_delay_ms=override_value(n->start_delay.mode,n->start_delay.delay_ms);r->end_delay_ms=override_value(n->end_delay.mode,n->end_delay.delay_ms);workflow_debug_log("Action runtime created: node='%s' start=%llu duration=%llu end=%llu",n->id,(unsigned long long)r->start_delay_ms,(unsigned long long)r->duration_ms,(unsigned long long)r->end_delay_ms);return r;}
void workflow_action_runtime_destroy(workflow_action_runtime_t*r){free(r);}
workflow_action_runtime_state_t workflow_action_runtime_state(const workflow_action_runtime_t*r){return r?r->state:WORKFLOW_ACTION_RUNTIME_COMPLETE;}
void workflow_action_runtime_begin_execution(workflow_action_runtime_t*r){if(!r||r->state!=WORKFLOW_ACTION_RUNTIME_WAIT_START_DELAY)return;r->state=WORKFLOW_ACTION_RUNTIME_EXECUTING;}
void workflow_action_runtime_complete_duration(workflow_action_runtime_t*r){if(r&&r->state==WORKFLOW_ACTION_RUNTIME_EXECUTING)r->state=WORKFLOW_ACTION_RUNTIME_WAIT_END_DELAY;}
void workflow_action_runtime_complete(workflow_action_runtime_t*r){if(r&&r->state==WORKFLOW_ACTION_RUNTIME_WAIT_END_DELAY)r->state=WORKFLOW_ACTION_RUNTIME_COMPLETE;}
workflow_t *workflow_action_runtime_workflow(const workflow_action_runtime_t*r){return r?r->workflow:nullptr;}
workflow_node_t *workflow_action_runtime_node(const workflow_action_runtime_t*r){return r?r->node:nullptr;}
uint64_t workflow_action_runtime_generation(const workflow_action_runtime_t*r){return r?r->generation:0;}
uint64_t workflow_action_runtime_duration_ms(const workflow_action_runtime_t*r){return r?r->duration_ms:0;}
uint64_t workflow_action_runtime_start_delay_ms(const workflow_action_runtime_t*r){return r?r->start_delay_ms:0;}
uint64_t workflow_action_runtime_end_delay_ms(const workflow_action_runtime_t*r){return r?r->end_delay_ms:0;}
bool workflow_action_runtime_has_duration(const workflow_action_runtime_t*r){return r&&r->duration_ms>0;}
