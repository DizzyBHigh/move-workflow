#include "workflow-action-executor.hpp"
#include "workflow-change-scene.h"
#include "workflow-debug.h"
#include "workflow-engine-runs.h"
#include "workflow-filter-instance.h"

bool workflow_action_executor_execute(workflow_engine_state_t *state, workflow_node_t *node)
{
    if (!state || !state->workflow || !node)
        return false;
    if (node->action.kind == WORKFLOW_CHANGE_SCENE)
        return workflow_change_scene_execute(node);

    workflow_engine_run_t *run = state->owner_run;
    workflow_filter_instance_set *set = workflow_engine_run_filter_instances(run);
    workflow_filter_instance *instance = workflow_filter_instance_set_get(set, node);
    if (!instance) {
        workflow_debug_log("Filter dispatch: no runtime instance for node='%s'", node->id);
        return false;
    }
    return workflow_filter_instance_execute(instance);
}
