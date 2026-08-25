#include "workflow-engine-runner.h"

#include "workflow-engine-node.h"

static bool run_node_and_links(workflow_engine_state_t *state, workflow_node_t *node)
{
    if (!workflow_engine_execute_node(state, node))
        return false;
    for (size_t i = 0; i < node->simultaneous_node_count; ++i) {
        workflow_node_t *branch = workflow_engine_find_node(
            state->workflow, node->simultaneous_node_ids[i]);
        if (branch && !workflow_engine_execute_node(state, branch))
            return false;
    }
    if (node->next_node_count == 0)
        return true;
    return workflow_engine_runner_run_node(state, node->next_node_ids[0]);
}

bool workflow_engine_runner_run_node(workflow_engine_state_t *state,
                                     const char *node_id)
{
    if (!workflow_engine_state_is_active(state) || !node_id)
        return false;
    workflow_node_t *node = workflow_engine_find_node(state->workflow, node_id);
    return node && run_node_and_links(state, node);
}

bool workflow_engine_runner_run_entries(workflow_engine_state_t *state)
{
    if (!workflow_engine_state_is_active(state) || !state->workflow)
        return false;
    bool executed = false;
    for (size_t i = 0; i < state->workflow->entry_node_count; ++i) {
        if (workflow_engine_runner_run_node(state,
                                            state->workflow->entry_node_ids[i]))
            executed = true;
    }
    return executed;
}
