#include "workflow-action-runtime.h"

#include "workflow-debug.h"

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

static uint64_t override_value(const workflow_value_mode_t mode,
                               const uint64_t value)
{
    return mode == WORKFLOW_OVERRIDE ? value : 0;
}

workflow_action_runtime_t *workflow_action_runtime_create(
    workflow_t *workflow, workflow_node_t *node, uint64_t generation)
{
    if (!workflow || !node || node->type != WORKFLOW_NODE_ACTION)
        return nullptr;
    workflow_action_runtime_t *runtime =
        (workflow_action_runtime_t *)calloc(1, sizeof(*runtime));
    if (!runtime)
        return nullptr;
    runtime->workflow = workflow;
    runtime->node = node;
    runtime->state = WORKFLOW_ACTION_RUNTIME_WAIT_START_DELAY;
    runtime->generation = generation;
    runtime->duration_ms = override_value(node->duration.mode,
                                          node->duration.duration_ms);
    runtime->start_delay_ms = override_value(node->start_delay.mode,
                                             node->start_delay.delay_ms);
    runtime->end_delay_ms = override_value(node->end_delay.mode,
                                           node->end_delay.delay_ms);
    workflow_debug_log("Action runtime created: node='%s' start=%llu duration=%llu end=%llu",
                       node->id, (unsigned long long)runtime->start_delay_ms,
                       (unsigned long long)runtime->duration_ms,
                       (unsigned long long)runtime->end_delay_ms);
    return runtime;
}

void workflow_action_runtime_destroy(workflow_action_runtime_t *runtime)
{
    free(runtime);
}

workflow_action_runtime_state_t workflow_action_runtime_state(
    const workflow_action_runtime_t *runtime)
{
    return runtime ? runtime->state : WORKFLOW_ACTION_RUNTIME_COMPLETE;
}

void workflow_action_runtime_begin_execution(workflow_action_runtime_t *runtime)
{
    if (!runtime || runtime->state != WORKFLOW_ACTION_RUNTIME_WAIT_START_DELAY)
        return;
    runtime->state = WORKFLOW_ACTION_RUNTIME_EXECUTING;
    workflow_debug_log("Action runtime executing: node='%s' duration=%llu",
                       runtime->node->id,
                       (unsigned long long)runtime->duration_ms);
}

void workflow_action_runtime_complete_duration(
    workflow_action_runtime_t *runtime)
{
    if (!runtime || runtime->state != WORKFLOW_ACTION_RUNTIME_EXECUTING)
        return;
    runtime->state = WORKFLOW_ACTION_RUNTIME_WAIT_END_DELAY;
    workflow_debug_log("Action runtime duration complete: node='%s'",
                       runtime->node->id);
}

void workflow_action_runtime_complete(workflow_action_runtime_t *runtime)
{
    if (!runtime || runtime->state != WORKFLOW_ACTION_RUNTIME_WAIT_END_DELAY)
        return;
    runtime->state = WORKFLOW_ACTION_RUNTIME_COMPLETE;
    workflow_debug_log("Action runtime complete: node='%s'", runtime->node->id);
}

workflow_t *workflow_action_runtime_workflow(const workflow_action_runtime_t *r)
{
    return r ? r->workflow : nullptr;
}

workflow_node_t *workflow_action_runtime_node(const workflow_action_runtime_t *r)
{
    return r ? r->node : nullptr;
}

uint64_t workflow_action_runtime_generation(const workflow_action_runtime_t *r)
{
    return r ? r->generation : 0;
}

uint64_t workflow_action_runtime_duration_ms(const workflow_action_runtime_t *r)
{
    return r ? r->duration_ms : 0;
}

uint64_t workflow_action_runtime_start_delay_ms(const workflow_action_runtime_t *r)
{
    return r ? r->start_delay_ms : 0;
}

uint64_t workflow_action_runtime_end_delay_ms(const workflow_action_runtime_t *r)
{
    return r ? r->end_delay_ms : 0;
}

bool workflow_action_runtime_has_duration(const workflow_action_runtime_t *r)
{
    return r && r->duration_ms > 0;
}
