#include "workflow-engine-state.h"

#include <cstring>

void workflow_engine_state_reset(workflow_engine_state_t *state)
{
    if (!state)
        return;
    bool running = state->running;
    unsigned long generation = state->generation;
    memset(state, 0, sizeof(*state));
    state->running = running;
    state->generation = generation;
}

void workflow_engine_state_begin(workflow_engine_state_t *state, workflow_t *workflow)
{
    if (!state)
        return;
    unsigned long generation = state->generation + 1;
    workflow_engine_state_reset(state);
    state->workflow = workflow;
    state->running = workflow != nullptr;
    state->generation = generation;
}

void workflow_engine_state_stop(workflow_engine_state_t *state)
{
    if (!state)
        return;
    state->stopping = true;
    state->running = false;
    state->pending_branches = 0;
    state->generation++;
}

bool workflow_engine_state_is_active(const workflow_engine_state_t *state)
{
    return state && state->running && !state->stopping;
}

void workflow_engine_state_delay_begin(workflow_engine_state_t *state)
{
    if (state)
        ++state->pending_branches;
}

void workflow_engine_state_delay_end(workflow_engine_state_t *state)
{
    if (!state || !state->pending_branches)
        return;
    --state->pending_branches;
    if (!state->pending_branches)
        workflow_engine_state_stop(state);
}
