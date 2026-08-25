#include "workflow-engine-node.h"

#include "workflow-debug.h"
#include "workflow-filter-instance.h"

#include <cstring>

workflow_node_t *workflow_engine_find_node(workflow_t *workflow, const char *node_id)
{
    if (!workflow || !node_id)
        return nullptr;
    for (size_t i = 0; i < workflow->node_count; ++i)
        if (strcmp(workflow->nodes[i].id, node_id) == 0)
            return &workflow->nodes[i];
    return nullptr;
}

bool workflow_engine_execute_node(workflow_engine_state_t *state,
                                  workflow_node_t *node)
{
    if (!workflow_engine_state_is_active(state) || !node)
        return false;
    strncpy(state->current_node_id, node->id, WORKFLOW_MAX_NAME - 1);
    state->current_node_id[WORKFLOW_MAX_NAME - 1] = '\0';
    workflow_debug_log("Execute node: %s (%s)", node->id,
                       workflow_node_type_name(node->type));
    if (node->type == WORKFLOW_NODE_TRIGGER) {
        workflow_debug_log("Trigger node reached: %s", node->id);
        return true;
    }
    const bool executed =
        workflow_filter_instance_execute_node(state->workflow, node);
    workflow_debug_log("Execute node result: %s result=%d", node->id,
                       executed ? 1 : 0);
    return executed;
}
