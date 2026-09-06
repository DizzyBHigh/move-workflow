#include "workflow-action-executor.hpp"
#include "workflow-change-scene.h"
#include "workflow-debug.h"
#include "workflow-engine-runs.h"
#include "workflow-filter-instance.h"

bool workflow_action_executor_execute(workflow_engine_state_t *state, workflow_node_t *node)
{
    if (!state || !state->workflow || !node)
        return false;

    workflow_debug_log(
        "Action dispatch: entering node='%s' type=%d action_kind=%d workflow='%s'",
        node->id, (int)node->type, (int)node->action.kind,
        state->workflow->name);

    if (node->action.kind == WORKFLOW_CHANGE_SCENE) {
        const bool result = workflow_change_scene_execute(node);
        workflow_debug_log(
            "Action dispatch: change-scene node='%s' result=%d",
            node->id, result);
        return result;
    }

    workflow_engine_run_t *run = state->owner_run;
    workflow_filter_instance_set *set = workflow_engine_run_filter_instances(run);
    workflow_debug_log(
        "Action dispatch: resolving runtime instance node='%s' run=%p set=%p",
        node->id, (void *)run, (void *)set);

    workflow_filter_instance *instance = workflow_filter_instance_set_get(set, node);
    if (!instance) {
        workflow_debug_log(
            "Action dispatch: NO runtime instance node='%s' scene='%s' filter='%s'",
            node->id, node->action.scene_name, node->action.filter_name);
        return false;
    }

    workflow_debug_log(
        "Action dispatch: runtime instance FOUND node='%s' instance=%p",
        node->id, (void *)instance);

    const bool result = workflow_filter_instance_execute(instance);
    workflow_debug_log(
        "Action dispatch: runtime execute node='%s' result=%d instance=%p",
        node->id, result, (void *)instance);
    return result;
}
