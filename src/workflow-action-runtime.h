#pragma once

#include "workflow-model.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum workflow_action_runtime_state {
    WORKFLOW_ACTION_RUNTIME_WAIT_START_DELAY = 0,
    WORKFLOW_ACTION_RUNTIME_EXECUTING,
    WORKFLOW_ACTION_RUNTIME_WAIT_END_DELAY,
    WORKFLOW_ACTION_RUNTIME_COMPLETE
} workflow_action_runtime_state_t;

typedef struct workflow_action_runtime workflow_action_runtime_t;

workflow_action_runtime_t *workflow_action_runtime_create(
    workflow_t *workflow,
    workflow_node_t *node,
    uint64_t generation);

void workflow_action_runtime_destroy(workflow_action_runtime_t *runtime);

workflow_action_runtime_state_t workflow_action_runtime_state(
    const workflow_action_runtime_t *runtime);

workflow_t *workflow_action_runtime_workflow(
    const workflow_action_runtime_t *runtime);

workflow_node_t *workflow_action_runtime_node(
    const workflow_action_runtime_t *runtime);

uint64_t workflow_action_runtime_generation(
    const workflow_action_runtime_t *runtime);

uint64_t workflow_action_runtime_duration_ms(
    const workflow_action_runtime_t *runtime);

uint64_t workflow_action_runtime_start_delay_ms(
    const workflow_action_runtime_t *runtime);

uint64_t workflow_action_runtime_end_delay_ms(
    const workflow_action_runtime_t *runtime);

bool workflow_action_runtime_has_duration(
    const workflow_action_runtime_t *runtime);

#ifdef __cplusplus
}
#endif
