#include "workflow-engine.h"

#include "workflow-engine-runner.h"
#include "workflow-engine-state.h"

#include <cstdlib>

struct workflow_engine {
    workflow_engine_state_t state;
};

workflow_engine_t *workflow_engine_create(void)
{
    return (workflow_engine_t *)calloc(1, sizeof(workflow_engine_t));
}

void workflow_engine_destroy(workflow_engine_t *engine)
{
    if (!engine)
        return;
    workflow_engine_stop(engine);
    free(engine);
}

bool workflow_engine_start(workflow_engine_t *engine, workflow_t *workflow)
{
    if (!engine || !workflow || workflow_engine_is_running(engine))
        return false;
    if (!workflow->enabled)
        return false;
    workflow_engine_state_begin(&engine->state, workflow);
    return true;
}

void workflow_engine_stop(workflow_engine_t *engine)
{
    if (!engine)
        return;
    workflow_engine_state_stop(&engine->state);
}

bool workflow_engine_is_running(const workflow_engine_t *engine)
{
    return engine && workflow_engine_state_is_active(&engine->state);
}

bool workflow_engine_run_entries(workflow_engine_t *engine)
{
    if (!engine)
        return false;
    return workflow_engine_runner_run_entries(&engine->state);
}
