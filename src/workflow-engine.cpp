#include "workflow-engine.h"
#include "workflow-debug.h"
#include "workflow-engine-node.h"
#include "workflow-engine-runner.h"
#include "workflow-engine-runs.h"
#include "workflow-engine-trigger.h"
#include <cstdlib>
#include <cstring>

struct workflow_engine { workflow_engine_runs_t *runs; };

workflow_engine_t *workflow_engine_create(void)
{
    workflow_engine_t *engine = (workflow_engine_t *)calloc(1, sizeof(*engine));
    if (!engine)
        return nullptr;
    engine->runs = workflow_engine_runs_create();
    if (!engine->runs) {
        free(engine);
        return nullptr;
    }
    return engine;
}

void workflow_engine_destroy(workflow_engine_t *engine)
{
    if (!engine)
        return;
    workflow_engine_stop(engine);
    workflow_engine_runs_destroy(engine->runs);
    free(engine);
}

bool workflow_engine_start(workflow_engine_t *engine, workflow_t *workflow)
{
    if (!engine || !workflow)
        return false;
    workflow_engine_run_t *run = workflow_engine_runs_start(engine->runs, workflow);
    return run != nullptr;
}

bool workflow_engine_start_trigger(workflow_engine_t *engine, workflow_t *workflow,
                                   const char *id)
{
    if (!engine || !workflow || !id || !*id || !workflow->enabled)
        return false;
    workflow_node_t *node = workflow_engine_find_node(workflow, id);
    if (!node || node->type != WORKFLOW_NODE_TRIGGER)
        return false;
    workflow_engine_run_t *run = workflow_engine_runs_start(engine->runs, workflow);
    if (!run)
        return false;
    workflow_debug_log("External trigger started workflow='%s' node='%s'", workflow->name, id);
    return workflow_engine_runner_run_node(workflow_engine_run_state(run), id);
}

void workflow_engine_stop(workflow_engine_t *engine)
{
    if (engine)
        workflow_engine_runs_stop_all(engine->runs);
}

bool workflow_engine_is_running(const workflow_engine_t *engine)
{
    return engine && workflow_engine_runs_any_active(engine->runs);
}

bool workflow_engine_is_workflow_running(const workflow_engine_t *engine, const char *id)
{
    if (!engine || !id)
        return false;
    for (workflow_engine_run_t *run = workflow_engine_runs_head(engine->runs); run;
         run = workflow_engine_run_next(run)) {
        const workflow_engine_state_t *state = workflow_engine_run_state_const(run);
        if (workflow_engine_state_is_active(state) && state->workflow &&
            !strcmp(state->workflow->id, id))
            return true;
    }
    return false;
}

static workflow_engine_state_t *current_state(workflow_engine_t *engine)
{
    return engine ? workflow_engine_run_state(workflow_engine_runs_current(engine->runs)) : nullptr;
}

bool workflow_engine_run_entries(workflow_engine_t *engine)
{
    return workflow_engine_runner_run_entries(current_state(engine));
}

bool workflow_engine_run_node(workflow_engine_t *engine, const char *id)
{
    return workflow_engine_runner_run_node(current_state(engine), id);
}

bool workflow_engine_test_node(workflow_engine_t *engine, workflow_t *workflow, const char *id)
{
    if (!engine || !workflow || !id || !*id)
        return false;
    workflow_engine_run_t *run = workflow_engine_runs_start(engine->runs, workflow);
    if (!run)
        return false;
    return workflow_engine_runner_run_node(workflow_engine_run_state(run), id);
}

bool workflow_engine_trigger(workflow_engine_t *engine, workflow_trigger_type_t type,
                             const char *value)
{
    return engine && workflow_engine_runs_trigger(engine->runs, type, value);
}
