#pragma once

#include "workflow-engine-state.h"

#ifdef __cplusplus
extern "C" {
#endif

bool workflow_engine_run_entries(workflow_engine_state_t *state);
bool workflow_engine_run_node(workflow_engine_state_t *state, const char *node_id);

#ifdef __cplusplus
}
#endif
