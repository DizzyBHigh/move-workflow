#include "workflow-shortcuts.h"

#include "workflow-engine-service.h"

#include <obs-module.h>
#include <string.h>

static workflow_t *pending_workflow;
static char pending_source[WORKFLOW_MAX_NAME];

void workflow_shortcuts_begin(workflow_t *workflow, const workflow_node_t *node)
{
    pending_workflow = NULL;
    pending_source[0] = '\0';
    if (!workflow || !node || node->shortcut_node_count == 0)
        return;

    pending_workflow = workflow;
    strncpy(pending_source, node->id, WORKFLOW_MAX_NAME - 1);
    pending_source[WORKFLOW_MAX_NAME - 1] = '\0';
    blog(LOG_INFO, "[Move Workflow] Shortcut waiting: workflow='%s' source='%s' targets=%zu",
         workflow->id, node->id, node->shortcut_node_count);
}

bool workflow_shortcuts_accept(workflow_t *workflow, const char *source_id,
                               const char *target_id)
{
    if (!workflow || workflow != pending_workflow || !source_id || !target_id)
        return false;
    if (strcmp(source_id, pending_source) != 0)
        return false;

    workflow_node_t *source = NULL;
    for (size_t i = 0; i < workflow->node_count; ++i) {
        if (strcmp(workflow->nodes[i].id, source_id) == 0) {
            source = &workflow->nodes[i];
            break;
        }
    }
    if (!source)
        return false;

    for (size_t i = 0; i < source->shortcut_node_count; ++i) {
        if (strcmp(source->shortcut_node_ids[i], target_id) != 0)
            continue;

        blog(LOG_INFO, "[Move Workflow] Shortcut accepted: workflow='%s' source='%s' target='%s'",
             workflow->id, source_id, target_id);
        const bool resumed = workflow_engine_service_resume_shortcut(
            workflow->id, source_id, target_id);
        if (resumed) {
            pending_workflow = NULL;
            pending_source[0] = '\0';
        } else {
            blog(LOG_WARNING,
                 "[Move Workflow] Shortcut resume failed: workflow='%s' source='%s' target='%s'",
                 workflow->id, source_id, target_id);
        }
        return resumed;
    }

    return false;
}

void workflow_shortcuts_cancel(void)
{
    pending_workflow = NULL;
    pending_source[0] = '\0';
}
