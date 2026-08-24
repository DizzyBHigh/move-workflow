#include "workflow-manager.h"

#include <string.h>

static void copy_name(char *dst, const char *src)
{
    if (!src)
        src = "";
    strncpy(dst, src, WORKFLOW_MAX_NAME - 1);
    dst[WORKFLOW_MAX_NAME - 1] = '\0';
}

void workflow_manager_init(workflow_manager_t *manager)
{
    if (!manager)
        return;
    memset(manager, 0, sizeof(*manager));
}

workflow_t *workflow_manager_find(workflow_manager_t *manager, const char *id)
{
    if (!manager || !id)
        return NULL;
    for (size_t i = 0; i < manager->workflow_count; ++i) {
        if (strcmp(manager->workflows[i].id, id) == 0)
            return &manager->workflows[i];
    }
    return NULL;
}

const workflow_t *workflow_manager_find_const(const workflow_manager_t *manager,
                                              const char *id)
{
    return workflow_manager_find((workflow_manager_t *)manager, id);
}

workflow_t *workflow_manager_create(workflow_manager_t *manager,
                                    const char *id, const char *name)
{
    if (!manager || !id || !id[0] || manager->workflow_count >= WORKFLOW_MANAGER_MAX_WORKFLOWS)
        return NULL;
    if (workflow_manager_find(manager, id))
        return NULL;

    workflow_t *workflow = &manager->workflows[manager->workflow_count++];
    memset(workflow, 0, sizeof(*workflow));
    copy_name(workflow->id, id);
    copy_name(workflow->name, name ? name : id);
    workflow->enabled = true;

    if (!manager->selected_workflow_id[0])
        copy_name(manager->selected_workflow_id, id);
    return workflow;
}

bool workflow_manager_remove(workflow_manager_t *manager, const char *id)
{
    if (!manager || !id)
        return false;
    for (size_t i = 0; i < manager->workflow_count; ++i) {
        if (strcmp(manager->workflows[i].id, id) != 0)
            continue;
        for (size_t j = i + 1; j < manager->workflow_count; ++j)
            manager->workflows[j - 1] = manager->workflows[j];
        --manager->workflow_count;
        if (strcmp(manager->selected_workflow_id, id) == 0) {
            manager->selected_workflow_id[0] = '\0';
            if (manager->workflow_count)
                copy_name(manager->selected_workflow_id, manager->workflows[0].id);
        }
        return true;
    }
    return false;
}

bool workflow_manager_set_enabled(workflow_manager_t *manager,
                                   const char *id, bool enabled)
{
    workflow_t *workflow = workflow_manager_find(manager, id);
    if (!workflow)
        return false;
    workflow->enabled = enabled;
    return true;
}

bool workflow_manager_set_selected(workflow_manager_t *manager, const char *id)
{
    if (!workflow_manager_find(manager, id))
        return false;
    copy_name(manager->selected_workflow_id, id);
    return true;
}

workflow_t *workflow_manager_selected(workflow_manager_t *manager)
{
    if (!manager || !manager->selected_workflow_id[0])
        return NULL;
    return workflow_manager_find(manager, manager->selected_workflow_id);
}

const workflow_t *workflow_manager_selected_const(const workflow_manager_t *manager)
{
    return workflow_manager_selected((workflow_manager_t *)manager);
}
