#include "workflow-engine-runner.h"

#include "workflow-debug.h"
#include "workflow-engine-delay.h"
#include "workflow-engine-node.h"
#include "workflow-engine-runner-actions.h"
#include "workflow-engine-runner-internal.h"

#include <cstdlib>
#include <cstring>

struct continuation {
    workflow_engine_state_t *state;
    char node_id[WORKFLOW_MAX_NAME];
    unsigned long generation;
    continuation_phase phase;
};

static uint64_t delay_value(workflow_value_mode_t mode, uint64_t value)
{
    return mode == WORKFLOW_OVERRIDE ? value : 0;
}

bool workflow_engine_runner_schedule_phase(workflow_engine_state_t *state, workflow_node_t *node,
                                           uint64_t delay_ms, continuation_phase phase)
{
    if (!delay_ms) return false;
    continuation *next = (continuation *)calloc(1, sizeof(*next));
    if (!next) return false;
    next->state = state;
    next->generation = state->generation;
    next->phase = phase;
    strncpy(next->node_id, node->id, WORKFLOW_MAX_NAME - 1);
    workflow_engine_state_delay_begin(state);
    workflow_debug_log("Action lifecycle: node='%s' scheduling %s for %llu ms",
                       node->id,
                       phase == PHASE_START_DELAY ? "start delay" :
                       phase == PHASE_DURATION ? "duration" :
                       phase == PHASE_END_DELAY ? "end delay" : "failed-action end delay",
                       (unsigned long long)delay_ms);
    if (!workflow_engine_delay_start(delay_ms, workflow_engine_runner_continue, next)) {
        workflow_engine_state_delay_end(state);
        free(next);
        return false;
    }
    return true;
}

void workflow_engine_runner_continue(void *data)
{
    continuation *next = (continuation *)data;
    if (!next) return;
    workflow_engine_state_t *state = next->state;
    if (workflow_engine_state_is_active(state) && state->generation == next->generation) {
        workflow_node_t *node = workflow_engine_find_node(state->workflow, next->node_id);
        if (node) {
            if (next->phase == PHASE_START_DELAY) {
                workflow_debug_log("Action lifecycle: start delay complete node='%s'", node->id);
                workflow_engine_runner_run_node_now(state, node, 0);
            } else if (next->phase == PHASE_DURATION) {
                workflow_debug_log("Action lifecycle: duration complete node='%s'", node->id);
                const uint64_t end_delay = delay_value(node->end_delay.mode, node->end_delay.delay_ms);
                if (!end_delay) workflow_engine_runner_run_next_links(state, node, 0);
                else if (!workflow_engine_runner_schedule_phase(state, node, end_delay, PHASE_END_DELAY))
                    workflow_engine_state_stop(state);
            } else {
                workflow_debug_log("Action lifecycle: %s complete node='%s'",
                                   next->phase == PHASE_FAILED_END_DELAY ? "failed-action end delay" : "end delay",
                                   node->id);
                workflow_engine_runner_run_next_links(state, node, 0);
            }
        }
    }
    workflow_engine_state_delay_end(state);
    free(next);
}

bool workflow_engine_runner_run_internal(workflow_engine_state_t *state, const char *node_id, size_t depth)
{
    if (!workflow_engine_state_is_active(state) || !node_id) return false;
    if (depth > WORKFLOW_MAX_NODES * WORKFLOW_MAX_LINKS) return false;
    workflow_node_t *node = workflow_engine_find_node(state->workflow, node_id);
    if (!node) return false;
    if (node->type == WORKFLOW_NODE_ACTION) {
        const uint64_t start_delay = delay_value(node->start_delay.mode, node->start_delay.delay_ms);
        if (start_delay)
            return workflow_engine_runner_schedule_phase(state, node, start_delay, PHASE_START_DELAY);
    }
    return workflow_engine_runner_run_node_now(state, node, depth);
}

bool workflow_engine_runner_run_node(workflow_engine_state_t *state, const char *node_id)
{
    return workflow_engine_runner_run_internal(state, node_id, 0);
}

bool workflow_engine_runner_run_entries(workflow_engine_state_t *state)
{
    if (!workflow_engine_state_is_active(state) || !state->workflow) return false;
    bool executed = false;
    for (size_t i = 0; i < state->workflow->entry_node_count; ++i)
        if (workflow_engine_runner_run_internal(state, state->workflow->entry_node_ids[i], 0)) executed = true;
    return executed;
}
