#pragma once

#include "workflow-engine-state.h"
#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

workflow_node_t *workflow_engine_find_node(workflow_t *workflow, const char *node_id);
bool workflow_engine_execute_node(workflow_engine_state_t *state, workflow_node_t *node);

#ifdef __cplusplus
}
#endif
