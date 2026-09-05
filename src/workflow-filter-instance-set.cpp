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
            workflow_debug_log("Filter instance: node='%s' has no valid Move filter; skipping preparation",
                               node->id);
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

    workflow_debug_log("Filter prepare: node='%s' scene='%s' filter='%s' filter_id='%s' kind=%d",
                       node->id, node->action.scene_name, node->action.filter_name,
                       node->action.filter_id, (int)node->action.kind);

    size_t existing = 0;
    if (find_index(set, node->id, &existing)) {
        workflow_debug_log("Filter prepare: node='%s' already maps to runtime index=%zu; possible duplicate node ID",
                           node->id, existing);
        return true;
    }
    if (set->count >= WORKFLOW_MAX_NODES)
        return false;

    obs_source_t *parent = obs_get_source_by_name(node->action.scene_name);
    if (!parent) {
        workflow_debug_log("Filter prepare: node='%s' scene='%s' NOT FOUND",
                           node->id, node->action.scene_name);
        return false;
    }

    obs_source_t *original = obs_source_get_filter_by_name(parent, node->action.filter_name);
    if (!original) {
        workflow_debug_log("Filter prepare: node='%s' filter='%s' NOT FOUND on scene='%s'",
                           node->id, node->action.filter_name, node->action.scene_name);
        obs_source_release(parent);
        return false;
    }

    const char *expected = workflow_expected_filter_id(node->action.kind);
    const char *actual = obs_source_get_id(original);
    workflow_debug_log("Filter prepare: node='%s' resolved filter='%s' actual_id='%s' expected_id='%s'",
                       node->id, obs_source_get_name(original), actual ? actual : "",
                       expected ? expected : "");
    if (!expected || !actual || strcmp(expected, actual)) {
        workflow_debug_log("Filter prepare: node='%s' REJECTED filter='%s' due to filter ID mismatch",
                           node->id, obs_source_get_name(original));
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
    workflow_debug_log("Filter prepare: node='%s' -> runtime='%s' runtime_id='%s' duration=%llu restore_delay=%llu",
                       node->id, obs_source_get_name(instance->instance),
                       obs_source_get_id(instance->instance),
                       (unsigned long long)duration,
                       (unsigned long long)restore_delay);
    return true;
}

workflow_filter_instance *workflow_filter_instance_set_get(
    workflow_filter_instance_set *set, const workflow_node_t *node)
{
    size_t index = 0;
    if (!node)
        return nullptr;
    const bool found = find_index(set, node->id, &index);
    workflow_debug_log("Filter lookup: node='%s' found=%s index=%zu",
                       node->id, found ? "true" : "false", index);
    return found ? set->instances[index] : nullptr;
}

void workflow_filter_instance_set_destroy(workflow_filter_instance_set *set)
{
    if (!set)
        return;
    for (size_t i = 0; i < set->count; ++i)
        workflow_filter_instance_destroy(set->instances[i]);
    free(set);
}
