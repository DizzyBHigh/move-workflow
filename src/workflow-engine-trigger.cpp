#include "workflow-engine-trigger.h"

#include "workflow-engine-runner.h"
#include "workflow-engine-node.h"

#include <cstring>

workflow_node_t *workflow_engine_find_trigger(workflow_t *workflow,
                                               workflow_trigger_type_t type,
                                               const char *value)
{
    if (!workflow)
        return nullptr;
    for (size_t i = 0; i < workflow->node_count; ++i) {
        workflow_node_t *node = &workflow->nodes[i];
        if (node->type != WORKFLOW_NODE_TRIGGER || node->trigger.type != type)
            continue;
        if (!value || strcmp(node->trigger.value, value) == 0)
            return node;
    }
    return nullptr;
}

bool workflow_engine_trigger(workflow_engine_state_t *state,
                             workflow_trigger_type_t type,
                             const char *value)
{
    if (!workflow_engine_state_is_active(state))
        return false;
    workflow_node_t *trigger = workflow_engine_find_trigger(state->workflow, type, value);
    if (!trigger)
        return false;
    return workflow_engine_run_node(state, trigger->id);
}
