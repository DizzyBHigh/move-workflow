#include "workflow-engine-service.h"

#include "workflow-engine.h"
#include "workflow-debug.h"

static workflow_engine_t *service_engine;

void workflow_engine_service_set(workflow_engine_t *engine)
{
    service_engine = engine;
}

bool workflow_engine_service_test_node(workflow_t *workflow, const char *node_id)
{
    if (!service_engine || !workflow || !node_id || !*node_id)
        return false;
    if (!workflow_engine_start(service_engine, workflow))
        return false;
    workflow_debug_log("TEST trigger node: %s", node_id);
    return workflow_engine_run_node(service_engine, node_id);
}
