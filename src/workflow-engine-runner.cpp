#include "workflow-engine-runner.h"

#include "workflow-action-runtime.h"
#include "workflow-debug.h"
#include "workflow-engine-delay.h"
#include "workflow-engine-node.h"

#include <cstdlib>
#include <cstring>

enum continuation_phase { PHASE_START_DELAY, PHASE_DURATION, PHASE_END_DELAY, PHASE_FAILED_END_DELAY };
struct continuation {
    workflow_engine_state_t *state;
    char node_id[WORKFLOW_MAX_NAME];
    unsigned long generation;
    continuation_phase phase;
};

static bool run_node_internal(workflow_engine_state_t *, const char *, size_t);
static bool run_node_now(workflow_engine_state_t *, workflow_node_t *, size_t);
static bool run_simultaneous(workflow_engine_state_t *, workflow_node_t *, size_t);
static bool run_next_links(workflow_engine_state_t *, workflow_node_t *, size_t);

static uint64_t delay_value(workflow_value_mode_t mode, uint64_t value)
{
    return mode == WORKFLOW_OVERRIDE ? value : 0;
}

static bool schedule_phase(workflow_engine_state_t *state, workflow_node_t *node,
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
    extern void workflow_engine_runner_continue(void *);
    if (!workflow_engine_delay_start(delay_ms, workflow_engine_runner_continue, next)) {
        workflow_engine_state_delay_end(state);
        free(next);
        return false;
    }
    return true;
}

static bool run_action(workflow_engine_state_t *state, workflow_node_t *node, size_t depth)
{
    const bool executed = workflow_engine_execute_node(state, node);
    if (!executed) {
        workflow_debug_log("Action lifecycle: node='%s' failed to execute; continuing workflow", node->id);
        const uint64_t end_delay = delay_value(node->end_delay.mode, node->end_delay.delay_ms);
        run_simultaneous(state, node, depth);
        if (end_delay) {
            if (schedule_phase(state, node, end_delay, PHASE_FAILED_END_DELAY)) return true;
            workflow_engine_state_stop(state);
            return false;
        }
        return run_next_links(state, node, depth);
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
        if (schedule_phase(state, node, duration, PHASE_DURATION)) return simultaneous_ok;
        return false;
    }
    if (end_delay) {
        if (schedule_phase(state, node, end_delay, PHASE_END_DELAY)) return simultaneous_ok;
        return false;
    }
    return simultaneous_ok && run_next_links(state, node, depth);
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
                run_node_now(state, node, 0);
            } else if (next->phase == PHASE_DURATION) {
                workflow_debug_log("Action lifecycle: duration complete node='%s'", node->id);
                const uint64_t end_delay = delay_value(node->end_delay.mode, node->end_delay.delay_ms);
                if (!end_delay) run_next_links(state, node, 0);
                else if (!schedule_phase(state, node, end_delay, PHASE_END_DELAY)) workflow_engine_state_stop(state);
            } else {
                workflow_debug_log("Action lifecycle: %s complete node='%s'",
                                   next->phase == PHASE_FAILED_END_DELAY ? "failed-action end delay" : "end delay",
                                   node->id);
                run_next_links(state, node, 0);
            }
        }
    }
    workflow_engine_state_delay_end(state);
    free(next);
}

static bool run_simultaneous(workflow_engine_state_t *state, workflow_node_t *node, size_t depth)
{
    bool result = true;
    for (size_t i = 0; i < node->simultaneous_node_count; ++i) {
        const char *child_id = node->simultaneous_node_ids[i];
        workflow_debug_log("Workflow graph: parent='%s' triggering simultaneous node='%s'",
                           node->id, child_id);
        if (!run_node_internal(state, child_id, depth + 1)) result = false;
    }
    return result;
}

static bool run_next_links(workflow_engine_state_t *state, workflow_node_t *node, size_t depth)
{
    if (node->next_node_count) {
        workflow_debug_log("Workflow graph: node='%s' completed; executing %zu next node(s)",
                           node->id, node->next_node_count);
        bool result = true;
        for (size_t i = 0; i < node->next_node_count; ++i)
            if (!run_node_internal(state, node->next_node_ids[i], depth + 1)) result = false;
        return result;
    }
    bool result = true;
    for (size_t i = 0; i < node->end_node_count; ++i)
        if (!run_node_internal(state, node->end_node_ids[i], depth + 1)) result = false;
    return result;
}

static bool run_node_now(workflow_engine_state_t *state, workflow_node_t *node, size_t depth)
{
    if (node->type == WORKFLOW_NODE_ACTION) return run_action(state, node, depth);
    if (!workflow_engine_execute_node(state, node)) return false;
    run_simultaneous(state, node, depth);
    return run_next_links(state, node, depth);
}

static bool run_node_internal(workflow_engine_state_t *state, const char *node_id, size_t depth)
{
    if (!workflow_engine_state_is_active(state) || !node_id) return false;
    if (depth > WORKFLOW_MAX_NODES * WORKFLOW_MAX_LINKS) return false;
    workflow_node_t *node = workflow_engine_find_node(state->workflow, node_id);
    if (!node) return false;
    if (node->type == WORKFLOW_NODE_ACTION) {
        const uint64_t start_delay = delay_value(node->start_delay.mode, node->start_delay.delay_ms);
        if (start_delay) return schedule_phase(state, node, start_delay, PHASE_START_DELAY);
    }
    return run_node_now(state, node, depth);
}

bool workflow_engine_runner_run_node(workflow_engine_state_t *state, const char *node_id)
{
    return run_node_internal(state, node_id, 0);
}

bool workflow_engine_runner_run_entries(workflow_engine_state_t *state)
{
    if (!workflow_engine_state_is_active(state) || !state->workflow) return false;
    bool executed = false;
    for (size_t i = 0; i < state->workflow->entry_node_count; ++i)
        if (run_node_internal(state, state->workflow->entry_node_ids[i], 0)) executed = true;
    return executed;
}
