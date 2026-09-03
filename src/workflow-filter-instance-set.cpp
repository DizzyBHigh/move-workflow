#include "workflow-filter-instance.h"

#include "workflow-debug.h"
#include "workflow-filter-settings.h"

#include <cstdlib>
#include <cstring>

struct workflow_filter_instance_set {
    workflow_t *workflow;
    workflow_filter_instance *instances[WORKFLOW_MAX_NODES];
    char node_ids[WORKFLOW_MAX_NODES][WORKFLOW_MAX_NAME];
    size_t count;
};

static bool find_index(const workflow_filter_instance_set *set,
                       const char *node_id, size_t *index)
{
    if (!set || !node_id)
        return false;
    for (size_t i = 0; i < set->count; ++i) {
        if (!strcmp(set->node_ids[i], node_id)) {
            if (index)
                *index = i;
            return true;
        }
    }
    return false;
}

workflow_filter_instance_set *workflow_filter_instance_set_create(workflow_t *workflow)
{
    if (!workflow || !workflow->enabled)
        return nullptr;
    workflow_filter_instance_set *set =
        (workflow_filter_instance_set *)calloc(1, sizeof(*set));
    if (!set)
        return nullptr;
    set->workflow = workflow;
    for (size_t i = 0; i < workflow->node_count; ++i) {
        workflow_node_t *node = &workflow->nodes[i];
        if (node->type == WORKFLOW_NODE_ACTION &&
            node->action.kind != WORKFLOW_CHANGE_SCENE &&
            !workflow_filter_instance_set_prepare_node(set, node)) {
            workflow_filter_instance_set_destroy(set);
            return nullptr;
        }
    }
    workflow_debug_log("Filter instances: prepared %zu runtime Move filters", set->count);
    return set;
}

bool workflow_filter_instance_set_prepare_node(workflow_filter_instance_set *set,
                                                workflow_node_t *node)
{
    if (!set || !node || node->type != WORKFLOW_NODE_ACTION ||
        node->action.kind == WORKFLOW_CHANGE_SCENE)
        return false;
    if (find_index(set, node->id, nullptr))
        return true;
    if (set->count >= WORKFLOW_MAX_NODES)
        return false;

    obs_source_t *parent = obs_get_source_by_name(node->action.scene_name);
    if (!parent)
        return false;
    obs_source_t *original = obs_source_get_filter_by_name(parent, node->action.filter_name);
    if (!original) {
        obs_source_release(parent);
        return false;
    }
    const char *expected = workflow_expected_filter_id(node->action.kind);
    const char *actual = obs_source_get_id(original);
    if (!expected || !actual || strcmp(expected, actual)) {
        obs_source_release(original);
        obs_source_release(parent);
        return false;
    }

    workflow_filter_instance *instance =
        workflow_filter_instance_create(original, parent, node);
    obs_source_release(original);
    obs_source_release(parent);
    if (!instance)
        return false;

    uint64_t duration = 0;
    uint64_t restore_delay = 0;
    workflow_filter_apply_node_settings(instance->instance, node, &duration, &restore_delay);
    set->instances[set->count] = instance;
    strncpy(set->node_ids[set->count], node->id, WORKFLOW_MAX_NAME - 1);
    ++set->count;
    workflow_debug_log("Filter instance: node='%s' prepared runtime='%s' duration=%llu",
                       node->id, obs_source_get_name(instance->instance),
                       (unsigned long long)duration);
    return true;
}

workflow_filter_instance *workflow_filter_instance_set_get(
    workflow_filter_instance_set *set, const workflow_node_t *node)
{
    size_t index = 0;
    return node && find_index(set, node->id, &index) ? set->instances[index] : nullptr;
}

void workflow_filter_instance_set_destroy(workflow_filter_instance_set *set)
{
    if (!set)
        return;
    for (size_t i = 0; i < set->count; ++i)
        workflow_filter_instance_destroy(set->instances[i]);
    free(set);
}
