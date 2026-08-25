#include "workflow-engine-service.h"

#include "workflow-debug.h"
#include "workflow-engine.h"
#include "workflow-persistence.h"

static workflow_engine_t *service_engine;

void workflow_engine_service_set(workflow_engine_t *engine)
{
    service_engine = engine;
}

bool workflow_engine_service_test_node(const char *workflow_id, const char *node_id)
{
    if (!service_engine || !workflow_id || !node_id || !*node_id)
        return false;
    workflow_manager_t *manager = workflow_persistence_manager();
    if (!manager)
        return false;
    workflow_t *workflow = workflow_manager_find(manager, workflow_id);
    if (!workflow || !workflow_engine_start(service_engine, workflow))
        return false;
    workflow_debug_log("TEST trigger node: %s", node_id);
    return workflow_engine_run_node(service_engine, node_id);
}
