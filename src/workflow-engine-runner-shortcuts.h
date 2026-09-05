#pragma once

#include "workflow-engine-state.h"
#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

bool workflow_engine_runner_wait_shortcut(workflow_engine_state_t *state,
                                          workflow_node_t *node);
bool workflow_engine_runner_activate_shortcut(workflow_engine_state_t *state,
                                              const char *source_id,
                                              const char *target_id);

#ifdef __cplusplus
}
#endif
