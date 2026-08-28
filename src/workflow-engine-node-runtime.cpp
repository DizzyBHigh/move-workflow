#include "workflow-engine-node-runtime.h"

#include <cstring>

void workflow_engine_node_runtime_reset(workflow_engine_node_runtime_t *runtime)
{
    if (!runtime)
        return;
    memset(runtime, 0, sizeof(*runtime));
    runtime->phase = WORKFLOW_NODE_PHASE_IDLE;
}

bool workflow_engine_node_runtime_is_active(
    const workflow_engine_node_runtime_t *runtime)
{
    return runtime && runtime->active && runtime->phase != WORKFLOW_NODE_PHASE_IDLE;
}

int64_t workflow_engine_node_runtime_remaining_ms(
    const workflow_engine_node_runtime_t *runtime,
    int64_t now_ms)
{
    if (!runtime || !workflow_engine_node_runtime_is_active(runtime))
        return 0;
    if (runtime->deadline_ms <= now_ms)
        return 0;
    return runtime->deadline_ms - now_ms;
}
