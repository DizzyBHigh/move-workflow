#include "workflow-engine-runner-actions.h"

#include "workflow-action-runtime.h"
#include "workflow-debug.h"
#include "workflow-engine-node.h"
#include "workflow-engine-runner-internal.h"
#include "workflow-engine-runner-shortcuts.h"

static bool run_simultaneous(workflow_engine_state_t *state, workflow_node_t *node, size_t depth)
{
    bool result = true;
    for (size_t i = 0; i < node->simultaneous_node_count; ++i) {
        const char *child_id = node->simultaneous_node_ids[i];
        workflow_debug_log("Workflow graph: parent='%s' triggering simultaneous node='%s'",
                           node->id, child_id);
        if (!workflow_engine_runner_run_internal(state, child_id, depth + 1)) result = false;
    }
    return result;
}

bool workflow_engine_runner_run_next_links(workflow_engine_state_t *state,
                                           workflow_node_t *node, size_t depth)
{
    if (node->shortcut_node_count)
        return workflow_engine_runner_wait_shortcut(state, node);
    if (node->next_node_count) {
        workflow_debug_log("Workflow graph: node='%s' completed; executing %zu next node(s)",
                           node->id, node->next_node_count);
        bool result = true;
        for (size_t i = 0; i < node->next_node_count; ++i)
            if (!workflow_engine_runner_run_internal(state, node->next_node_ids[i], depth + 1)) result = false;
        return result;
    }
    bool result = true;
    for (size_t i = 0; i < node->end_node_count; ++i)
        if (!workflow_engine_runner_run_internal(state, node->end_node_ids[i], depth + 1)) result = false;
    return result;
}

static bool run_action(workflow_engine_state_t *state, workflow_node_t *node, size_t depth)
{
    const bool executed = workflow_engine_execute_node(state, node);
    if (!executed) {
        workflow_debug_log("Action lifecycle: node='%s' failed to execute; continuing workflow", node->id);
        const uint64_t end_delay = node->end_delay.mode == WORKFLOW_OVERRIDE ? node->end_delay.delay_ms : 0;
        run_simultaneous(state, node, depth);
        if (end_delay) {
            if (workflow_engine_runner_schedule_phase(state, node, end_delay, PHASE_FAILED_END_DELAY)) return true;
            workflow_engine_state_stop(state);
            return false;
        }
        return workflow_engine_runner_run_next_links(state, node, depth);
    }

    const bool simultaneous_ok = run_simultaneous(state, node, depth);
    workflow_action_runtime_t *runtime =
        workflow_action_runtime_create(state->workflow, node, state->generation);
    if (!runtime) return false;
    workflow_action_runtime_begin_execution(runtime);
    const uint64_t duration = workflow_action_runtime_duration_ms(runtime);
    const uint64_t end_delay = workflow_action_runtime_end_delay_ms(runtime);
    workflow_action_runtime_destroy(runtime);
    if (duration) {
        if (workflow_engine_runner_schedule_phase(state, node, duration, PHASE_DURATION)) return simultaneous_ok;
        return false;
    }
    if (end_delay) {
        if (workflow_engine_runner_schedule_phase(state, node, end_delay, PHASE_END_DELAY)) return simultaneous_ok;
        return false;
    }
    return simultaneous_ok && workflow_engine_runner_run_next_links(state, node, depth);
}

bool workflow_engine_runner_run_node_now(workflow_engine_state_t *state,
                                         workflow_node_t *node, size_t depth)
{
    if (node->type == WORKFLOW_NODE_ACTION) return run_action(state, node, depth);
    if (!workflow_engine_execute_node(state, node)) return false;
    run_simultaneous(state, node, depth);
    return workflow_engine_runner_run_next_links(state, node, depth);
}
