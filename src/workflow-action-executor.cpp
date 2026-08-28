#include "workflow-action-executor.hpp"
#include "workflow-change-scene.h"
#include "workflow-filter-instance.h"

bool workflow_action_executor_execute(workflow_t *workflow, workflow_node_t *node)
{
    if (!workflow || !node)
        return false;
    if (node->action.kind == WORKFLOW_CHANGE_SCENE)
        return workflow_change_scene_execute(node);
    return workflow_filter_instance_execute_node(workflow, node);
}
