#include "workflow-engine.h"

#include "workflow-debug.h"
#include "workflow-engine-runner.h"
#include "workflow-engine-runs.h"
#include "workflow-engine-trigger.h"

#include <cstdlib>

struct workflow_engine {
    workflow_engine_runs_t *runs;
};

workflow_engine_t *workflow_engine_create(void)
{
    workflow_engine_t *engine =
        (workflow_engine_t *)calloc(1, sizeof(workflow_engine_t));
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
    workflow_debug_log("Workflow run start: workflow='%s' result=%d",
                       workflow->name, run ? 1 : 0);
    return run != nullptr;
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

static workflow_engine_state_t *current_state(workflow_engine_t *engine)
{
    if (!engine)
        return nullptr;
    workflow_engine_run_t *run = workflow_engine_runs_current(engine->runs);
    return workflow_engine_run_state(run);
}

bool workflow_engine_run_entries(workflow_engine_t *engine)
{
    return workflow_engine_runner_run_entries(current_state(engine));
}

bool workflow_engine_run_node(workflow_engine_t *engine, const char *node_id)
{
    return workflow_engine_runner_run_node(current_state(engine), node_id);
}

bool workflow_engine_test_node(workflow_engine_t *engine,
                               workflow_t *workflow,
                               const char *node_id)
{
    if (!engine || !workflow || !node_id || !*node_id)
        return false;
    workflow_debug_log("TEST engine: creating independent run for workflow='%s' node='%s'.",
                       workflow->name, node_id);
    workflow_engine_run_t *run = workflow_engine_runs_start(engine->runs, workflow);
    if (!run) {
        workflow_debug_log("TEST engine FAILED: run allocation/start failed.");
        return false;
    }
    const bool result =
        workflow_engine_runner_run_node(workflow_engine_run_state(run), node_id);
    workflow_debug_log("TEST engine: node='%s' execution result=%d.",
                       node_id, result ? 1 : 0);
    return result;
}

bool workflow_engine_trigger(workflow_engine_t *engine,
                             workflow_trigger_type_t type,
                             const char *value)
{
    if (!engine)
        return false;
    return workflow_engine_runs_trigger(engine->runs, type, value);
}
