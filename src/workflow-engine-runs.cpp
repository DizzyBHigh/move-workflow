#include "workflow-engine-runs.h"

#include <cstdlib>

struct workflow_engine_run {
    workflow_engine_state_t state;
    workflow_engine_run_t *next;
};

struct workflow_engine_runs {
    workflow_engine_run_t *head;
    workflow_engine_run_t *current;
};

workflow_engine_runs_t *workflow_engine_runs_create(void)
{
    return (workflow_engine_runs_t *)calloc(1, sizeof(workflow_engine_runs_t));
}

void workflow_engine_runs_destroy(workflow_engine_runs_t *runs)
{
    if (!runs)
        return;
    workflow_engine_run_t *run = runs->head;
    while (run) {
        workflow_engine_run_t *next = run->next;
        free(run);
        run = next;
    }
    free(runs);
}

workflow_engine_run_t *workflow_engine_runs_start(workflow_engine_runs_t *runs,
                                                   workflow_t *workflow)
{
    if (!runs || !workflow || !workflow->enabled)
        return nullptr;
    workflow_engine_run_t *run =
        (workflow_engine_run_t *)calloc(1, sizeof(workflow_engine_run_t));
    if (!run)
        return nullptr;
    workflow_engine_state_begin(&run->state, workflow);
    run->next = runs->head;
    runs->head = run;
    runs->current = run;
    return run;
}

workflow_engine_state_t *workflow_engine_run_state(workflow_engine_run_t *run)
{
    return run ? &run->state : nullptr;
}

workflow_engine_run_t *workflow_engine_runs_current(workflow_engine_runs_t *runs)
{
    return runs ? runs->current : nullptr;
}

void workflow_engine_runs_stop_all(workflow_engine_runs_t *runs)
{
    if (!runs)
        return;
    for (workflow_engine_run_t *run = runs->head; run; run = run->next)
        workflow_engine_state_stop(&run->state);
}

bool workflow_engine_runs_any_active(const workflow_engine_runs_t *runs)
{
    if (!runs)
        return false;
    for (const workflow_engine_run_t *run = runs->head; run; run = run->next)
        if (workflow_engine_state_is_active(&run->state))
            return true;
    return false;
}
