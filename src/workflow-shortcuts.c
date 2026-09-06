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
    blog(LOG_INFO,
         "[Move Workflow] Shortcut callback: workflow='%s' source='%s' target='%s' pending_workflow=%p pending_source='%s'",
         workflow ? workflow->id : "", source_id ? source_id : "", target_id ? target_id : "",
         (void *)pending_workflow, pending_source);

    if (!workflow || workflow != pending_workflow || !source_id || !target_id) {
        blog(LOG_INFO, "[Move Workflow] Shortcut callback rejected: pending state mismatch");
        return false;
    }
    if (strcmp(source_id, pending_source) != 0) {
        blog(LOG_INFO,
             "[Move Workflow] Shortcut callback rejected: source mismatch expected='%s' actual='%s'",
             pending_source, source_id);
        return false;
    }

    workflow_node_t *source = NULL;
    for (size_t i = 0; i < workflow->node_count; ++i) {
        if (strcmp(workflow->nodes[i].id, source_id) == 0) {
            source = &workflow->nodes[i];
            break;
        }
    }
    if (!source) {
        blog(LOG_WARNING, "[Move Workflow] Shortcut callback rejected: source node not found");
        return false;
    }

    blog(LOG_INFO,
         "[Move Workflow] Shortcut callback source resolved: source='%s' targets=%zu bindings=%zu",
         source->id, source->shortcut_node_count, source->shortcut_binding_count);
    for (size_t i = 0; i < source->shortcut_node_count; ++i) {
        const char *candidate_id = source->shortcut_node_ids[i];
        const char *key = "";
        for (size_t j = 0; j < source->shortcut_binding_count; ++j) {
            if (strcmp(source->shortcut_bindings[j].target_id, candidate_id) == 0) {
                key = source->shortcut_bindings[j].key;
                break;
            }
        }
        blog(LOG_INFO, "[Move Workflow] Shortcut callback candidate[%zu]: target='%s' key='%s' match=%d",
             i, candidate_id, key, strcmp(candidate_id, target_id) == 0);
    }

    for (size_t i = 0; i < source->shortcut_node_count; ++i) {
        if (strcmp(source->shortcut_node_ids[i], target_id) != 0)
            continue;

        blog(LOG_INFO, "[Move Workflow] Shortcut accepted: workflow='%s' source='%s' target='%s'",
             workflow->id, source_id, target_id);
        const bool resumed = workflow_engine_service_accept_shortcut(workflow->id, source_id, target_id);
        blog(LOG_INFO,
             "[Move Workflow] Shortcut callback resume result: workflow='%s' source='%s' target='%s' resumed=%d",
             workflow->id, source_id, target_id, resumed);
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

    blog(LOG_INFO, "[Move Workflow] Shortcut callback rejected: target is not registered on source");
    return false;
}

void workflow_shortcuts_cancel(void)
{
    pending_workflow = NULL;
    pending_source[0] = '\0';
}
