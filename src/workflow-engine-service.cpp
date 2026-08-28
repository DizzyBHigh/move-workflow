#include "workflow-engine-service.h"
#include "workflow-engine.h"
#include "workflow-engine-node.h"
#include "workflow-persistence.h"
#include <obs-module.h>

static workflow_engine_t *service_engine;

void workflow_engine_service_set(workflow_engine_t *engine)
{
    service_engine = engine;
    blog(LOG_INFO, "[Move Workflow] Engine service %s.", engine ? "connected" : "disconnected");
}

static workflow_t *find_workflow(const char *id)
{
    auto *manager = workflow_persistence_manager();
    return manager && id ? workflow_manager_find(manager, id) : nullptr;
}

bool workflow_engine_service_trigger(const char *workflow_id, const char *trigger_id)
{
    if (!service_engine || !workflow_id || !trigger_id) return false;
    auto *workflow = find_workflow(workflow_id);
    if (!workflow) return false;
    auto *node = workflow_engine_find_node(workflow, trigger_id);
    if (!node || node->type != WORKFLOW_NODE_TRIGGER) return false;
    return workflow_engine_start_trigger(service_engine, workflow, node->id);
}

bool workflow_engine_service_trigger_scene(const char *) { return false; }

bool workflow_engine_service_workflow_running(const char *workflow_id)
{
    return service_engine && workflow_id && workflow_engine_is_workflow_running(service_engine, workflow_id);
}

bool workflow_engine_service_test_node(const char *workflow_id, const char *node_id)
{
    if (!service_engine || !workflow_id || !node_id) return false;
    auto *workflow = find_workflow(workflow_id);
    if (!workflow || !workflow->enabled) return false;
    return workflow_engine_test_node(service_engine, workflow, node_id);
}

bool workflow_engine_service_node_runtime(const char *workflow_id, const char *node_id,
                                          workflow_engine_node_runtime_t *out)
{
    return service_engine && workflow_id && node_id && out &&
           workflow_engine_get_node_runtime(service_engine, workflow_id, node_id, out);
}
