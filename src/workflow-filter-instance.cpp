#include "workflow-filter-instance.h"

#include "workflow-debug.h"

#include <cstdlib>
#include <pthread.h>
#include <util/platform.h>

struct restore_context {
    obs_source_t *source;
    obs_data_t *settings;
    uint64_t delay_ms;
};

static void restore_on_ui(void *data)
{
    auto *context = static_cast<restore_context *>(data);
    if (!context)
        return;
    if (context->source) {
        workflow_debug_log("Filter instance: restoring native Move filter '%s'",
                           obs_source_get_name(context->source));
        obs_source_set_enabled(context->source, false);
        if (context->settings)
            obs_source_update(context->source, context->settings);
        obs_source_release(context->source);
    }
    if (context->settings)
        obs_data_release(context->settings);
    free(context);
}

static void *restore_thread(void *data)
{
    auto *context = static_cast<restore_context *>(data);
    if (!context)
        return nullptr;
    os_sleep_ms((uint32_t)context->delay_ms);
    obs_queue_task(OBS_TASK_UI, restore_on_ui, context, false);
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

    if (!instance->restore_delay_ms)
        return true;
    auto *context = (restore_context *)calloc(1, sizeof(*context));
    if (!context)
        return true;
    context->source = obs_source_get_ref(instance->instance);
    context->settings = obs_data_get_json(instance->restore_settings)
        ? obs_data_create_from_json(obs_data_get_json(instance->restore_settings)) : nullptr;
    context->delay_ms = instance->restore_delay_ms;
    pthread_t thread;
    if (pthread_create(&thread, nullptr, restore_thread, context) == 0)
        pthread_detach(thread);
    else {
        obs_source_release(context->source);
        if (context->settings) obs_data_release(context->settings);
        free(context);
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
