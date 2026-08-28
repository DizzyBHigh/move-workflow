#pragma once

#include <cstddef>
#include <cstdint>

#include "workflow-engine-state.h"
#include "workflow-model.h"

enum continuation_phase {
    PHASE_START_DELAY,
    PHASE_DURATION,
    PHASE_END_DELAY,
    PHASE_FAILED_END_DELAY
};

bool workflow_engine_runner_schedule_phase(workflow_engine_state_t *state,
                                           workflow_node_t *node,
                                           uint64_t delay_ms,
                                           continuation_phase phase);

bool workflow_engine_runner_run_internal(workflow_engine_state_t *state,
                                         const char *node_id,
                                         size_t depth);

bool workflow_engine_runner_run_node_now(workflow_engine_state_t *state,
                                         workflow_node_t *node,
                                         size_t depth);
