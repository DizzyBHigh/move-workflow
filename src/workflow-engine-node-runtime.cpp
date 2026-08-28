#include "workflow-engine-node-runtime.h"

#include <cstring>

void workflow_engine_node_runtime_reset(workflow_engine_node_runtime_t *runtime)
{
    if (!runtime)
        return;
    memset(runtime, 0, sizeof(*runtime));
    runtime->phase = WORKFLOW_NODE_RUNTIME_IDLE;
}

void workflow_engine_node_runtime_begin(
    workflow_engine_node_runtime_t *runtime,
    const char *node_id,
    workflow_node_runtime_phase_t phase,
    uint64_t deadline_ms)
{
    if (!runtime)
        return;
    workflow_engine_node_runtime_reset(runtime);
    if (node_id)
        strncpy(runtime->node_id, node_id, sizeof(runtime->node_id) - 1);
    runtime->phase = phase;
    runtime->deadline_ms = deadline_ms;
}

void workflow_engine_node_runtime_set_phase(
    workflow_engine_node_runtime_t *runtime,
    workflow_node_runtime_phase_t phase,
    uint64_t deadline_ms)
{
    if (!runtime)
        return;
    runtime->phase = phase;
    runtime->deadline_ms = deadline_ms;
}

bool workflow_engine_node_runtime_is_active(
    const workflow_engine_node_runtime_t *runtime)
{
    return runtime && runtime->phase != WORKFLOW_NODE_RUNTIME_IDLE;
}

uint64_t workflow_engine_node_runtime_remaining_ms(
    const workflow_engine_node_runtime_t *runtime,
    uint64_t now_ms)
{
    if (!runtime || !workflow_engine_node_runtime_is_active(runtime))
        return 0;
    return runtime->deadline_ms > now_ms ? runtime->deadline_ms - now_ms : 0;
}
