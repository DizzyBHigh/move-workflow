#include "workflow-engine-trigger.h"
#include "workflow-engine-runner.h"

workflow_node_t *workflow_engine_find_trigger(workflow_t *, workflow_trigger_type_t, const char *)
{
    return nullptr;
}

bool workflow_engine_trigger_dispatch(workflow_engine_state_t *, workflow_trigger_type_t, const char *)
{
    return false;
}
