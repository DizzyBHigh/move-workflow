#pragma once

#include "workflow-engine-state.h"
#include "workflow-model.h"

bool workflow_action_executor_execute(workflow_engine_state_t *state,
                                      workflow_node_t *node);
