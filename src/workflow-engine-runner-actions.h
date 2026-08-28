#pragma once

#include <cstddef>

#include "workflow-engine-state.h"
#include "workflow-model.h"

bool workflow_engine_runner_run_node_now(workflow_engine_state_t *state,
                                         workflow_node_t *node,
                                         size_t depth);
