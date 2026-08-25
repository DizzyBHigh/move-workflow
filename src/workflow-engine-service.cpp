#include "workflow-engine-service.h"

#include "workflow-debug.h"
#include "workflow-engine.h"
#include "workflow-engine-node.h"
#include "workflow-persistence.h"

#include <obs-module.h>

static workflow_engine_t *service_engine;

void workflow_engine_service_set(workflow_engine_t *engine)
{
    service_engine = engine;
}

bool workflow_engine_service_test_node(const char *workflow_id, const char *node_id)
{
    if (!service_engine || !workflow_id || !node_id || !*node_id) {
        blog(LOG_WARNING, "[Move Workflow] Trigger test failed: invalid arguments.");
        return false;
    }
    workflow_manager_t *manager = workflow_persistence_manager();
    if (!manager) {
        blog(LOG_WARNING, "[Move Workflow] Trigger test failed: workflow manager unavailable.");
        return false;
    }
    workflow_t *workflow = workflow_manager_find(manager, workflow_id);
    if (!workflow) {
        blog(LOG_WARNING, "[Move Workflow] Trigger test failed: workflow '%s' not found.",
             workflow_id);
        return false;
    }
    workflow_debug_log("TEST start: workflow='%s' enabled=%d node='%s'",
                       workflow->name, workflow->enabled ? 1 : 0, node_id);
    workflow_node_t *node = workflow_engine_find_node(workflow, node_id);
    if (!node) {
        blog(LOG_WARNING, "[Move Workflow] Trigger test failed: node '%s' not found.",
             node_id);
        return false;
    }
    if (!workflow->enabled) {
        blog(LOG_WARNING, "[Move Workflow] Trigger test failed: workflow '%s' is disabled.",
             workflow->name);
        return false;
    }
    blog(LOG_INFO, "[Move Workflow] Trigger test starting: workflow='%s' node='%s'.",
         workflow->name, node_id);
    const bool result = workflow_engine_test_node(service_engine, workflow, node_id);
    workflow_debug_log("TEST result: node='%s' result=%d", node_id, result ? 1 : 0);
    if (result)
        blog(LOG_INFO, "[Move Workflow] Trigger test started successfully: workflow='%s' node='%s'.",
             workflow->name, node_id);
    else
        blog(LOG_WARNING, "[Move Workflow] Trigger test failed while running node '%s'.", node_id);
    return result;
}
