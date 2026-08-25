#pragma once

#include "workflow-engine-state.h"
#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

workflow_node_t *workflow_engine_find_trigger(workflow_t *workflow,
                                               workflow_trigger_type_t type,
                                               const char *value);
bool workflow_engine_trigger_dispatch(workflow_engine_state_t *state,
                                      workflow_trigger_type_t type,
                                      const char *value);

#ifdef __cplusplus
}
#endif
