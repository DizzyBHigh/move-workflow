#include "workflow-shortcuts.h"

#include "workflow-engine-service.h"

void workflow_shortcuts_begin(workflow_t *workflow, const workflow_node_t *node)
{
    (void)workflow;
    (void)node;
}

bool workflow_shortcuts_accept(workflow_t *workflow, const char *source_id,
                               const char *target_id)
{
    if (!workflow || !source_id || !target_id)
        return false;
    return workflow_engine_service_accept_shortcut(workflow->id, source_id, target_id);
}

void workflow_shortcuts_cancel(void)
{
}
