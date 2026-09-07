#include "workflow-engine-service.h"
#include "workflow-engine.h"
#include "workflow-engine-node.h"
#include "workflow-persistence.h"
#include "workflow-debug.h"
#include <obs-module.h>

static workflow_engine_t *service_engine;

void workflow_engine_service_set(workflow_engine_t *engine)
{
    service_engine = engine;
    blog(LOG_INFO, "[Move Workflow] Engine service %s.", engine ? "connected" : "disconnected");
}

workflow_engine_t *workflow_engine_service_engine(void)
{
    return service_engine;
}

static workflow_t *find_workflow(const char *id)
{
    auto *manager = workflow_persistence_manager();
    return manager && id ? workflow_manager_find(manager, id) : nullptr;
}

bool workflow_engine_service_trigger(const char *workflow_id, const char *trigger_id)
{
    workflow_debug_log("Trigger service: request workflow='%s' trigger='%s' engine=%p",
                       workflow_id ? workflow_id : "", trigger_id ? trigger_id : "",
                       (void *)service_engine);
    if (!service_engine || !workflow_id || !trigger_id) return false;
    auto *workflow = find_workflow(workflow_id);
    if (!workflow) return false;
    auto *node = workflow_engine_find_node(workflow, trigger_id);
    if (!node || node->type != WORKFLOW_NODE_TRIGGER) return false;
    return workflow_engine_start_trigger(service_engine, workflow, node->id);
}

bool workflow_engine_service_accept_shortcut(const char *workflow_id, const char *source_id,
                                             const char *target_id)
{
    if (!service_engine || !workflow_id || !source_id || !target_id) return false;
    auto *workflow = find_workflow(workflow_id);
    if (!workflow || !workflow->enabled) return false;
    return workflow_engine_accept_shortcut(service_engine, workflow, source_id, target_id);
}

bool workflow_engine_service_trigger_scene(const char *) { return false; }

bool workflow_engine_service_workflow_running(const char *workflow_id)
{
    return service_engine && workflow_id && workflow_engine_is_workflow_running(service_engine, workflow_id);
}

bool workflow_engine_service_stop_workflow(const char *workflow_id)
{
    return service_engine && workflow_id && workflow_engine_stop_workflow(service_engine, workflow_id);
}

bool workflow_engine_service_stop_run(const char *workflow_id, uint64_t run_id)
{
    if (!service_engine || !workflow_id || !*workflow_id || !run_id) return false;
    auto *workflow = find_workflow(workflow_id);
    if (!workflow) return false;
    return workflow_engine_stop_run(service_engine, run_id);
}

bool workflow_engine_service_test_node(const char *workflow_id, const char *node_id)
{
    if (!service_engine || !workflow_id || !node_id) return false;
    auto *workflow = find_workflow(workflow_id);
    if (!workflow || !workflow->enabled) return false;
    return workflow_engine_test_node(service_engine, workflow, node_id);
}

bool workflow_engine_service_run_from_node(const char *workflow_id, const char *node_id)
{
    if (!service_engine || !workflow_id || !node_id || !*node_id) return false;
    auto *workflow = find_workflow(workflow_id);
    if (!workflow || !workflow->enabled) return false;
    return workflow_engine_test_node(service_engine, workflow, node_id);
}

bool workflow_engine_service_resume_shortcut(const char *workflow_id,
                                             const char *source_id, const char *target_id)
{
    if (!service_engine || !workflow_id || !source_id || !target_id) return false;
    auto *workflow = find_workflow(workflow_id);
    if (!workflow || !workflow->enabled) return false;
    return workflow_engine_accept_shortcut(service_engine, workflow, source_id, target_id);
}

bool workflow_engine_service_node_runtime(const char *workflow_id, const char *node_id,
                                          workflow_engine_node_runtime_t *out)
{
    return service_engine && workflow_id && node_id && out &&
           workflow_engine_get_node_runtime(service_engine, workflow_id, node_id, out);
}
