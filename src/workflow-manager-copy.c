#include "workflow-manager-copy.h"
#include "workflow-node-identity.hpp"

#include <string.h>

static void copy_text(char *dst, const char *src)
{
    strncpy(dst, src ? src : "", WORKFLOW_MAX_NAME - 1);
    dst[WORKFLOW_MAX_NAME - 1] = '\0';
}

static void remap_id(char *id, const char old_ids[][WORKFLOW_MAX_NAME],
                     const char new_ids[][WORKFLOW_MAX_NAME], size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(id, old_ids[i]) == 0) {
            copy_text(id, new_ids[i]);
            return;
        }
    }
}

static void remap_links(workflow_t *workflow, const char old_ids[][WORKFLOW_MAX_NAME],
                        const char new_ids[][WORKFLOW_MAX_NAME], size_t count)
{
    for (size_t i = 0; i < workflow->node_count; ++i) {
        workflow_node_t *node = &workflow->nodes[i];
        char (*groups[])[WORKFLOW_MAX_NAME] = {node->end_node_ids,
            node->simultaneous_node_ids, node->next_node_ids, node->shortcut_node_ids};
        size_t counts[] = {node->end_node_count, node->simultaneous_node_count,
                           node->next_node_count, node->shortcut_node_count};
        for (size_t g = 0; g < 4; ++g)
            for (size_t j = 0; j < counts[g]; ++j)
                remap_id(groups[g][j], old_ids, new_ids, count);
    }
    for (size_t i = 0; i < workflow->entry_node_count; ++i)
        remap_id(workflow->entry_node_ids[i], old_ids, new_ids, count);
}

workflow_t *workflow_manager_duplicate(workflow_manager_t *manager,
                                       const char *source_id, const char *new_id,
                                       const char *new_name)
{
    if (!manager || !source_id || !new_id || !new_id[0] ||
        manager->workflow_count >= WORKFLOW_MANAGER_MAX_WORKFLOWS ||
        workflow_manager_find(manager, new_id))
        return NULL;
    const workflow_t *source = workflow_manager_find_const(manager, source_id);
    if (!source || source->node_count > WORKFLOW_MAX_NODES)
        return NULL;

    char old_ids[WORKFLOW_MAX_NODES][WORKFLOW_MAX_NAME] = {};
    char new_ids[WORKFLOW_MAX_NODES][WORKFLOW_MAX_NAME] = {};
    workflow_t *copy = &manager->workflows[manager->workflow_count++];
    *copy = *source;
    copy_text(copy->id, new_id);
    if (new_name && new_name[0])
        copy_text(copy->name, new_name);

    for (size_t i = 0; i < copy->node_count; ++i) {
        copy_text(old_ids[i], source->nodes[i].id);
        if (!workflow_manager_generate_node_id(manager, new_ids[i],
                                                WORKFLOW_MAX_NAME)) {
            --manager->workflow_count;
            return NULL;
        }
        copy_text(copy->nodes[i].id, new_ids[i]);
    }
    remap_links(copy, old_ids, new_ids, copy->node_count);
    copy_text(manager->selected_workflow_id, copy->id);
    return copy;
}
