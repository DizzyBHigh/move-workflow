#include "workflow-engine-runner-actions.h"

#include "workflow-action-runtime.h"
#include "workflow-debug.h"
#include "workflow-engine-runner.h"

static bool run_action(workflow_engine_state_t *state, workflow_node_t *node, size_t depth)
{
    if (!workflow_action_runtime_start(state, node)) {
        workflow_debug_log("Runner: action start failed");
        return false;
    }
    workflow_debug_log("Runner: action started");
    return workflow_engine_runner_run_node_now(state, node, depth);
}

static bool run_simultaneous(workflow_engine_state_t *state, workflow_node_t *node, size_t depth)
{
    for (size_t i = 0; i < node->simultaneous_count; ++i) {
        workflow_node_t *linked = workflow_engine_state_find_node(state, node->simultaneous[i]);
        if (!linked)
            continue;
        if (!workflow_engine_runner_run_node_now(state, linked, depth + 1))
            return false;
    }
    return true;
}

static bool run_next_links(workflow_engine_state_t *state, workflow_node_t *node, size_t depth)
{
    for (size_t i = 0; i < node->next_count; ++i) {
        workflow_node_t *linked = workflow_engine_state_find_node(state, node->next[i]);
        if (!linked)
            continue;
        if (!workflow_engine_runner_run_node_now(state, linked, depth + 1))
            return false;
    }
    return true;
}

bool workflow_engine_runner_run_node_now(workflow_engine_state_t *state,
                                         workflow_node_t *node,
                                         size_t depth)
{
    if (!state || !node || depth > 64)
        return false;
    if (!run_action(state, node, depth))
        return false;
    if (!run_simultaneous(state, node, depth))
        return false;
    return run_next_links(state, node, depth);
}

bool workflow_engine_runner_run_action(workflow_engine_state_t *state,
                                       workflow_node_t *node,
                                       size_t depth)
{
    return workflow_engine_runner_run_node_now(state, node, depth);
}
