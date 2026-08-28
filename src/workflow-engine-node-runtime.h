#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum workflow_engine_node_phase {
    WORKFLOW_NODE_PHASE_IDLE = 0,
    WORKFLOW_NODE_PHASE_START_DELAY,
    WORKFLOW_NODE_PHASE_EXECUTION,
    WORKFLOW_NODE_PHASE_END_DELAY,
} workflow_engine_node_phase_t;

typedef struct workflow_engine_node_runtime {
    char node_id[128];
    workflow_engine_node_phase_t phase;
    int64_t deadline_ms;
    bool active;
} workflow_engine_node_runtime_t;

void workflow_engine_node_runtime_reset(workflow_engine_node_runtime_t *runtime);

bool workflow_engine_node_runtime_is_active(
    const workflow_engine_node_runtime_t *runtime);

int64_t workflow_engine_node_runtime_remaining_ms(
    const workflow_engine_node_runtime_t *runtime,
    int64_t now_ms);

#ifdef __cplusplus
}
#endif
