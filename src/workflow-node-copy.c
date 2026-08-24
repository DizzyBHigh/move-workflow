#include "workflow-node-copy.h"

#include <string.h>

bool workflow_node_duplicate(workflow_t *workflow, const char *node_id,
                             const char *new_id, const char *new_name)
{
    if (!workflow || !node_id || !new_id || !new_id[0])
        return false;
    if (workflow->node_count >= WORKFLOW_MAX_NODES)
        return false;

    for (size_t i = 0; i < workflow->node_count; ++i) {
        if (strcmp(workflow->nodes[i].id, new_id) == 0)
            return false;
        if (strcmp(workflow->nodes[i].id, node_id) != 0)
            continue;

        workflow_node_t copy = workflow->nodes[i];
        strncpy(copy.id, new_id, WORKFLOW_MAX_NAME - 1);
        copy.id[WORKFLOW_MAX_NAME - 1] = '\0';
        if (new_name && new_name[0]) {
            strncpy(copy.name, new_name, WORKFLOW_MAX_NAME - 1);
            copy.name[WORKFLOW_MAX_NAME - 1] = '\0';
        }
        copy.end_node_count = 0;
        copy.simultaneous_node_count = 0;
        copy.next_node_count = 0;
        copy.shortcut_node_count = 0;
        workflow->nodes[workflow->node_count++] = copy;
        return true;
    }
    return false;
}
