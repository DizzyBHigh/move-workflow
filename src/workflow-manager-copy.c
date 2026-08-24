#include "workflow-manager-copy.h"

#include <string.h>

static void copy_text(char *dst, const char *src)
{
    strncpy(dst, src ? src : "", WORKFLOW_MAX_NAME - 1);
    dst[WORKFLOW_MAX_NAME - 1] = '\0';
}

static void remap_links(workflow_t *workflow, const char *old_id, const char *new_id)
{
    for (size_t i = 0; i < workflow->node_count; ++i) {
        workflow_node_t *node = &workflow->nodes[i];
        char (*groups[])[WORKFLOW_MAX_NAME] = {
            node->end_node_ids, node->simultaneous_node_ids,
            node->next_node_ids, node->shortcut_node_ids};
        size_t counts[] = {node->end_node_count, node->simultaneous_node_count,
                           node->next_node_count, node->shortcut_node_count};
        for (size_t g = 0; g < 4; ++g)
            for (size_t j = 0; j < counts[g]; ++j)
                if (strcmp(groups[g][j], old_id) == 0)
                    copy_text(groups[g][j], new_id);
    }
    for (size_t i = 0; i < workflow->entry_node_count; ++i)
        if (strcmp(workflow->entry_node_ids[i], old_id) == 0)
            copy_text(workflow->entry_node_ids[i], new_id);
}

workflow_t *workflow_manager_duplicate(workflow_manager_t *manager,
                                       const char *source_id,
                                       const char *new_id,
                                       const char *new_name)
{
    if (!manager || !source_id || !new_id || !new_id[0] ||
        manager->workflow_count >= WORKFLOW_MANAGER_MAX_WORKFLOWS ||
        workflow_manager_find(manager, new_id))
        return NULL;

    const workflow_t *source = workflow_manager_find_const(manager, source_id);
    if (!source)
        return NULL;

    workflow_t *copy = &manager->workflows[manager->workflow_count++];
    *copy = *source;
    copy_text(copy->id, new_id);
    if (new_name && new_name[0])
        copy_text(copy->name, new_name);
    manager->selected_workflow_id[0] = '\0';
    copy_text(manager->selected_workflow_id, copy->id);
    return copy;
}
