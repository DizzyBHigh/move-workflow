#include "workflow-filter-instance.h"

#include "workflow-debug.h"

#include <cstdio>
#include <cstdlib>

workflow_filter_instance *workflow_filter_instance_create(
    obs_source_t *original, obs_source_t *parent, const workflow_node_t *node)
{
    if (!original || !parent || !node)
        return nullptr;
    workflow_filter_instance *result =
        (workflow_filter_instance *)calloc(1, sizeof(*result));
    if (!result)
        return nullptr;

    char name[WORKFLOW_MAX_NAME];
    snprintf(name, sizeof(name), "%s [workflow:%p]",
             obs_source_get_name(original), (void *)result);
    result->instance = obs_source_duplicate(original, name, true);
    if (!result->instance) {
        free(result);
        return nullptr;
    }
    result->original = obs_source_get_ref(original);
    result->parent = obs_source_get_ref(parent);
    obs_source_set_enabled(result->instance, false);
    obs_source_filter_add(parent, result->instance);

    workflow_debug_log("Filter instance: duplicated '%s' -> '%s' node='%s'",
                       obs_source_get_name(original), obs_source_get_name(result->instance),
                       node->id);
    return result;
}

static void enable_source_on_ui(void *data)
{
    obs_source_t *source = (obs_source_t *)data;
    if (!source)
        return;
    obs_source_set_enabled(source, true);
    obs_source_release(source);
}

bool workflow_filter_instance_execute(workflow_filter_instance *instance)
{
    if (!instance || !instance->instance)
        return false;
    obs_source_t *source = obs_source_get_ref(instance->instance);
    if (!source)
        return false;
    obs_queue_task(OBS_TASK_UI, enable_source_on_ui, source, false);
    workflow_debug_log("Filter instance: queued enable '%s'",
                       obs_source_get_name(instance->instance));
    return true;
}

void workflow_filter_instance_destroy(workflow_filter_instance *instance)
{
    if (!instance)
        return;
    if (instance->parent && instance->instance)
        obs_source_filter_remove(instance->parent, instance->instance);
    if (instance->instance)
        obs_source_release(instance->instance);
    if (instance->parent)
        obs_source_release(instance->parent);
    if (instance->original)
        obs_source_release(instance->original);
    free(instance);
}
