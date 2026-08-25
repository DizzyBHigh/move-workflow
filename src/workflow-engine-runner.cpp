#include "workflow-engine-runner.h"

#include "workflow-debug.h"
#include "workflow-engine-delay.h"
#include "workflow-engine-node.h"

#include <cstdlib>
#include <cstring>

struct continuation {
    workflow_engine_state_t *state;
    char node_id[WORKFLOW_MAX_NAME];
    unsigned long generation;
    bool run_node;
};

static bool run_node_internal(workflow_engine_state_t *state,
                              const char *node_id, size_t depth);
static bool run_links(workflow_engine_state_t *state, workflow_node_t *node,
                      size_t depth);

static void continue_run(void *data)
{
    continuation *next = (continuation *)data;
    if (!next)
        return;
    if (workflow_engine_state_is_active(next->state) &&
        next->state->generation == next->generation) {
        workflow_debug_log("Resume delayed node: %s", next->node_id);
        workflow_node_t *node = workflow_engine_find_node(
            next->state->workflow, next->node_id);
        if (node) {
            if (next->run_node)
                run_node_internal(next->state, next->node_id, 0);
            else
                run_links(next->state, node, 1);
        }
    }
    workflow_engine_state_delay_end(next->state);
    free(next);
}

static bool schedule(workflow_engine_state_t *state, workflow_node_t *node,
                     uint64_t delay_ms, bool run_node)
{
    continuation *next = (continuation *)calloc(1, sizeof(*next));
    if (!next)
        return false;
    next->state = state;
    next->generation = state->generation;
    next->run_node = run_node;
    strncpy(next->node_id, node->id, WORKFLOW_MAX_NAME - 1);
    workflow_engine_state_delay_begin(state);
    workflow_debug_log("Delay node %s for %llu ms", node->id,
                       (unsigned long long)delay_ms);
    if (!workflow_engine_delay_start(delay_ms, continue_run, next)) {
        workflow_engine_state_delay_end(state);
        free(next);
        return false;
    }
    return true;
}

static bool run_links(workflow_engine_state_t *state, workflow_node_t *node,
                      size_t depth)
{
    for (size_t i = 0; i < node->simultaneous_node_count; ++i)
        if (!run_node_internal(state, node->simultaneous_node_ids[i], depth + 1))
            return false;
    if (node->next_node_count) {
        for (size_t i = 0; i < node->next_node_count; ++i)
            if (!run_node_internal(state, node->next_node_ids[i], depth + 1))
                return false;
        return true;
    }
    for (size_t i = 0; i < node->end_node_count; ++i)
        if (!run_node_internal(state, node->end_node_ids[i], depth + 1))
            return false;
    return true;
}

static bool run_node_now(workflow_engine_state_t *state, workflow_node_t *node,
                         size_t depth)
{
    if (!workflow_engine_execute_node(state, node))
        return false;
    return node->end_delay.mode == WORKFLOW_OVERRIDE && node->end_delay.delay_ms
        ? schedule(state, node, node->end_delay.delay_ms, false)
        : run_links(state, node, depth);
}

static bool run_node_internal(workflow_engine_state_t *state,
                              const char *node_id, size_t depth)
{
    if (!workflow_engine_state_is_active(state) || !node_id)
        return false;
    if (depth > WORKFLOW_MAX_NODES * WORKFLOW_MAX_LINKS)
        return false;
    workflow_node_t *node = workflow_engine_find_node(state->workflow, node_id);
    if (!node)
        return false;
    return node->start_delay.mode == WORKFLOW_OVERRIDE && node->start_delay.delay_ms
        ? schedule(state, node, node->start_delay.delay_ms, true)
        : run_node_now(state, node, depth);
}

bool workflow_engine_runner_run_node(workflow_engine_state_t *state,
                                     const char *node_id)
{
    bool result = run_node_internal(state, node_id, 0);
    if (result && state && !state->pending_branches)
        workflow_engine_state_stop(state);
    return result;
}

bool workflow_engine_runner_run_entries(workflow_engine_state_t *state)
{
    if (!workflow_engine_state_is_active(state) || !state->workflow)
        return false;
    workflow_debug_log("Run %zu workflow entries", state->workflow->entry_node_count);
    bool executed = false;
    for (size_t i = 0; i < state->workflow->entry_node_count; ++i)
        if (run_node_internal(state, state->workflow->entry_node_ids[i], 0))
            executed = true;
    if (executed && !state->pending_branches)
        workflow_engine_state_stop(state);
    return executed;
}
