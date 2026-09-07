#include "workflow-engine-runs.h"
#include "workflow-debug.h"
#include "workflow-filter-instance.h"
#include <chrono>
#include <cstdlib>
#include <cstring>

struct workflow_engine_run {
    workflow_engine_state_t state;
    workflow_engine_run_t *next;
    uint64_t id;
    size_t references;
    workflow_filter_instance_set *filter_instances;
};
struct workflow_engine_runs { workflow_engine_run_t *head; workflow_engine_run_t *current; uint64_t next_id; };

static int64_t now_ms(void)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

workflow_engine_runs_t *workflow_engine_runs_create(void)
{ return (workflow_engine_runs_t *)calloc(1, sizeof(workflow_engine_runs_t)); }

void workflow_engine_run_retain(workflow_engine_run_t *run)
{ if (run) ++run->references; }

void workflow_engine_run_release(workflow_engine_run_t *run)
{
    if (!run || !run->references) return;
    --run->references;
    if (!run->references) {
        workflow_filter_instance_set_destroy(run->filter_instances);
        free(run);
    }
}

void workflow_engine_runs_destroy(workflow_engine_runs_t *runs)
{
    if (!runs) return;
    for (auto *run = runs->head; run;) {
        auto *next = run->next;
        run->next = nullptr;
        workflow_engine_run_release(run);
        run = next;
    }
    free(runs);
}

workflow_engine_run_t *workflow_engine_runs_start(workflow_engine_runs_t *runs, workflow_t *workflow)
{
    if (!runs || !workflow || !workflow->enabled) return nullptr;
    auto *run = (workflow_engine_run_t *)calloc(1, sizeof(workflow_engine_run_t));
    if (!run) return nullptr;
    run->references = 1;
    run->id = ++runs->next_id;
    run->filter_instances = workflow_filter_instance_set_create(workflow);
    if (!run->filter_instances) { free(run); return nullptr; }
    workflow_engine_state_begin(&run->state, workflow);
    run->state.owner_run = run;
    run->next = runs->head;
    runs->head = run;
    runs->current = run;
    workflow_debug_log("Run %llu created: workflow='%s' with %s runtime filters",
                       (unsigned long long)run->id, workflow->name,
                       run->filter_instances ? "prepared" : "no");
    return run;
}

workflow_engine_state_t *workflow_engine_run_state(workflow_engine_run_t *run)
{ return run ? &run->state : nullptr; }

const workflow_engine_state_t *workflow_engine_run_state_const(const workflow_engine_run_t *run)
{ return run ? &run->state : nullptr; }

workflow_engine_run_t *workflow_engine_run_next(const workflow_engine_run_t *run)
{ return run ? run->next : nullptr; }

workflow_engine_run_t *workflow_engine_runs_head(workflow_engine_runs_t *runs)
{ return runs ? runs->head : nullptr; }

workflow_engine_run_t *workflow_engine_runs_current(workflow_engine_runs_t *runs)
{ return runs ? runs->current : nullptr; }

uint64_t workflow_engine_run_id(const workflow_engine_run_t *run)
{ return run ? run->id : 0; }

bool workflow_engine_run_get_info(const workflow_engine_run_t *run,
                                  workflow_engine_run_info_t *out)
{
    if (!run || !out) return false;
    const workflow_engine_state_t *state = &run->state;
    out->run_id = run->id;
    out->workflow_id = state->workflow ? state->workflow->id : nullptr;
    out->workflow_name = state->workflow ? state->workflow->name : nullptr;
    out->current_node_id = state->current_node_id[0] ? state->current_node_id : nullptr;
    out->phase = WORKFLOW_NODE_PHASE_IDLE;
    out->elapsed_ms = 0;
    out->duration_ms = 0;
    if (out->current_node_id) {
        const auto *runtime = workflow_engine_state_node_runtime_const(state, out->current_node_id);
        if (runtime && workflow_engine_node_runtime_is_active(runtime)) {
            out->phase = runtime->phase;
            out->duration_ms = runtime->deadline_ms - runtime->start_ms;
            out->elapsed_ms = now_ms() - runtime->start_ms;
            if (out->elapsed_ms < 0) out->elapsed_ms = 0;
            if (out->duration_ms < 0) out->duration_ms = 0;
        }
    }
    out->running = workflow_engine_state_is_active(state);
    out->waiting_for_shortcut = state->waiting_for_shortcut;
    return true;
}

workflow_engine_run_t *workflow_engine_runs_find(workflow_engine_runs_t *runs, uint64_t run_id)
{
    if (!runs || !run_id) return nullptr;
    for (auto *run = runs->head; run; run = run->next)
        if (run->id == run_id) return run;
    return nullptr;
}

workflow_engine_run_t *workflow_engine_runs_find_shortcut(workflow_engine_runs_t *runs,
                                                           const char *workflow_id,
                                                           const char *source_id)
{
    if (!runs || !workflow_id || !*workflow_id || !source_id || !*source_id) return nullptr;
    for (auto *run = runs->head; run; run = run->next) {
        const workflow_engine_state_t *state = &run->state;
        if (!workflow_engine_state_is_active(state) || !state->waiting_for_shortcut) continue;
        if (!state->workflow || std::strcmp(state->workflow->id, workflow_id)) continue;
        if (std::strcmp(state->shortcut_source_id, source_id)) continue;
        return run;
    }
    return nullptr;
}

workflow_filter_instance_set *workflow_engine_run_filter_instances(workflow_engine_run_t *run)
{ return run ? run->filter_instances : nullptr; }

void workflow_engine_run_filter_instances_cleanup(workflow_engine_run_t *run)
{
    if (!run || !run->filter_instances) return;
    workflow_filter_instance_set_destroy(run->filter_instances);
    run->filter_instances = nullptr;
}

void workflow_engine_runs_stop_all(workflow_engine_runs_t *runs)
{
    if (!runs) return;
    for (auto *run = runs->head; run; run = run->next)
        workflow_engine_state_stop(&run->state);
}

size_t workflow_engine_runs_stop_workflow(workflow_engine_runs_t *runs, const char *workflow_id)
{
    if (!runs || !workflow_id || !*workflow_id) return 0;
    size_t stopped = 0;
    for (auto *run = runs->head; run; run = run->next) {
        workflow_engine_state_t *state = &run->state;
        if (!workflow_engine_state_is_active(state) || !state->workflow) continue;
        if (std::strcmp(state->workflow->id, workflow_id)) continue;
        workflow_engine_state_stop(state);
        ++stopped;
    }
    return stopped;
}

bool workflow_engine_runs_any_active(const workflow_engine_runs_t *runs)
{
    if (!runs) return false;
    for (auto *run = runs->head; run; run = run->next)
        if (workflow_engine_state_is_active(&run->state)) return true;
    return false;
}
