#include "workflow-filter-instance.h"

#include "workflow-debug.h"

#include <cstdlib>
#include <pthread.h>
#include <util/platform.h>

static void restore_on_ui(void *data)
{
    auto *instance = static_cast<workflow_filter_instance *>(data);
    if (!instance)
        return;
    workflow_debug_log("Filter instance: restoring native Move filter '%s'",
                       obs_source_get_name(instance->instance));
    if (instance->instance) {
        obs_source_set_enabled(instance->instance, false);
        if (instance->restore_settings)
            obs_source_update(instance->instance, instance->restore_settings);
    }
}

static void *restore_thread(void *data)
{
    auto *instance = static_cast<workflow_filter_instance *>(data);
    if (!instance)
        return nullptr;
    os_sleep_ms((uint32_t)instance->restore_delay_ms);
    obs_queue_task(OBS_TASK_UI, restore_on_ui, instance, false);
    return nullptr;
}

workflow_filter_instance *workflow_filter_instance_create(
    obs_source_t *original, obs_source_t *parent, const workflow_node_t *node)
{
    if (!original || !parent || !node)
        return nullptr;
    auto *result =
        (workflow_filter_instance *)calloc(1, sizeof(*result));
    if (!result)
        return nullptr;

    result->original = obs_source_get_ref(original);
    result->instance = obs_source_get_ref(original);
    result->parent = obs_source_get_ref(parent);
    result->restore_settings = obs_source_get_settings(original);
    workflow_debug_log("Filter instance: using original '%s' for node='%s'",
                       obs_source_get_name(original), node->id);
    return result;
}

bool workflow_filter_instance_execute(workflow_filter_instance *instance)
{
    if (!instance || !instance->instance)
        return false;
    obs_source_set_enabled(instance->instance, true);
    workflow_debug_log("Filter instance: enabled native Move filter '%s'",
                       obs_source_get_name(instance->instance));
    if (instance->restore_delay_ms) {
        pthread_t thread;
        if (pthread_create(&thread, nullptr, restore_thread, instance) == 0)
            pthread_detach(thread);
    }
    return true;
}

void workflow_filter_instance_destroy(workflow_filter_instance *instance)
{
    if (!instance)
        return;
    if (instance->restore_settings) {
        obs_data_release(instance->restore_settings);
        instance->restore_settings = nullptr;
    }
    if (instance->instance)
        obs_source_release(instance->instance);
    if (instance->parent)
        obs_source_release(instance->parent);
    if (instance->original)
        obs_source_release(instance->original);
    free(instance);
}
