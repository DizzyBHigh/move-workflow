#include "workflow-engine.h"

#include <cstdlib>
#include <cstring>

struct workflow_engine {
    workflow_t *workflow;
    bool running;
};

workflow_engine_t *workflow_engine_create(void)
{
    workflow_engine_t *engine = (workflow_engine_t *)calloc(1, sizeof(*engine));
    return engine;
}

void workflow_engine_destroy(workflow_engine_t *engine)
{
    if (!engine)
        return;
    free(engine);
}

bool workflow_engine_start(workflow_engine_t *engine, workflow_t *workflow)
{
    if (!engine || !workflow || engine->running)
        return false;

    engine->workflow = workflow;
    engine->running = true;
    return true;
}

void workflow_engine_stop(workflow_engine_t *engine)
{
    if (!engine)
        return;

    engine->running = false;
    engine->workflow = NULL;
}

bool workflow_engine_is_running(const workflow_engine_t *engine)
{
    return engine && engine->running;
}
