#include "workflow-action-runtime.h"
#include "workflow-change-scene.h"
#include "workflow-debug.h"
#include "workflow-node-timing-defaults.h"
#include <cstdlib>

struct workflow_action_runtime {
    workflow_t *workflow;
    workflow_node_t *node;
    workflow_action_runtime_state_t state;
    uint64_t generation;
    uint64_t duration_ms;
    uint64_t start_delay_ms;
    uint64_t end_delay_ms;
};

static uint64_t resolve_value(workflow_value_mode_t mode, uint64_t value, uint64_t existing)
{
    return mode == WORKFLOW_OVERRIDE ? value : existing;
}

workflow_action_runtime_t *workflow_action_runtime_create(workflow_t *workflow,
                                                          workflow_node_t *node,
                                                          uint64_t generation)
{
    if (!workflow || !node || node->type != WORKFLOW_NODE_ACTION) return nullptr;
    auto *runtime = (workflow_action_runtime_t *)calloc(1, sizeof(*runtime));
    if (!runtime) return nullptr;
    runtime->workflow = workflow; runtime->node = node;
    runtime->state = WORKFLOW_ACTION_RUNTIME_WAIT_START_DELAY;
    runtime->generation = generation;
    const auto defaults = workflow_node_read_timing_defaults(
        node->action.scene_name, node->action.filter_name);
    const uint64_t start_default = defaults.valid ? defaults.start_delay_ms : 0;
    const uint64_t duration_default = defaults.valid ? defaults.duration_ms : 0;
    const uint64_t end_default = defaults.valid ? defaults.end_delay_ms : 0;
    runtime->duration_ms = resolve_value(node->duration.mode, node->duration.duration_ms,
                                         duration_default);
    if (node->action.kind == WORKFLOW_CHANGE_SCENE &&
        node->action.scene_completion == WORKFLOW_SCENE_COMPLETE_TRANSITION)
        runtime->duration_ms = workflow_change_scene_transition_duration();
    runtime->start_delay_ms = resolve_value(node->start_delay.mode, node->start_delay.delay_ms,
                                            start_default);
    runtime->end_delay_ms = resolve_value(node->end_delay.mode, node->end_delay.delay_ms,
                                          end_default);
    workflow_debug_log("Action runtime created: node='%s' start=%llu duration=%llu end=%llu",
                       node->id, (unsigned long long)runtime->start_delay_ms,
                       (unsigned long long)runtime->duration_ms,
                       (unsigned long long)runtime->end_delay_ms);
    return runtime;
}

void workflow_action_runtime_destroy(workflow_action_runtime_t *runtime) { free(runtime); }
workflow_action_runtime_state_t workflow_action_runtime_state(const workflow_action_runtime_t *runtime)
{ return runtime ? runtime->state : WORKFLOW_ACTION_RUNTIME_COMPLETE; }
void workflow_action_runtime_begin_execution(workflow_action_runtime_t *runtime)
{ if (runtime && runtime->state == WORKFLOW_ACTION_RUNTIME_WAIT_START_DELAY) runtime->state = WORKFLOW_ACTION_RUNTIME_EXECUTING; }
void workflow_action_runtime_complete_duration(workflow_action_runtime_t *runtime)
{ if (runtime && runtime->state == WORKFLOW_ACTION_RUNTIME_EXECUTING) runtime->state = WORKFLOW_ACTION_RUNTIME_WAIT_END_DELAY; }
void workflow_action_runtime_complete(workflow_action_runtime_t *runtime)
{ if (runtime && runtime->state == WORKFLOW_ACTION_RUNTIME_WAIT_END_DELAY) runtime->state = WORKFLOW_ACTION_RUNTIME_COMPLETE; }
workflow_t *workflow_action_runtime_workflow(const workflow_action_runtime_t *runtime)
{ return runtime ? runtime->workflow : nullptr; }
workflow_node_t *workflow_action_runtime_node(const workflow_action_runtime_t *runtime)
{ return runtime ? runtime->node : nullptr; }
uint64_t workflow_action_runtime_generation(const workflow_action_runtime_t *runtime)
{ return runtime ? runtime->generation : 0; }
uint64_t workflow_action_runtime_duration_ms(const workflow_action_runtime_t *runtime)
{ return runtime ? runtime->duration_ms : 0; }
uint64_t workflow_action_runtime_start_delay_ms(const workflow_action_runtime_t *runtime)
{ return runtime ? runtime->start_delay_ms : 0; }
uint64_t workflow_action_runtime_end_delay_ms(const workflow_action_runtime_t *runtime)
{ return runtime ? runtime->end_delay_ms : 0; }
bool workflow_action_runtime_has_duration(const workflow_action_runtime_t *runtime)
{ return runtime && runtime->duration_ms > 0; }
