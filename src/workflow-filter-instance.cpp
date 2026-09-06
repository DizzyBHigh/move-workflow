#include "workflow-filter-instance.h"

#include "workflow-debug.h"
#include "workflow-filter-diagnostics.hpp"

#include <cstdio>
#include <cstdlib>
#include <obs.h>

namespace {
struct deferred_enable {
    obs_source_t *filter;
};

static void enable_filter_task(void *param)
{
    auto *task = static_cast<deferred_enable *>(param);
    if (!task)
        return;

    if (task->filter && !obs_source_removed(task->filter)) {
        workflow_debug_log("Filter instance: deferred enable '%s'",
                           obs_source_get_name(task->filter));
        obs_source_set_enabled(task->filter, true);
    }

    if (task->filter)
        obs_source_release(task->filter);
    free(task);
}
}

static void log_move_settings(obs_source_t *source, const char *stage)
{
    if (!source)
        return;
    obs_data_t *settings = obs_source_get_settings(source);
    if (!settings)
        return;
    workflow_debug_log(
        "Filter instance: %s name='%s' trigger=%lld source='%s' "
        "duration=%lld duration_type=%lld custom_duration=%d "
        "enabled_match_moving=%d easing=%lld easing_function=%lld "
        "simultaneous='%s' next='%s' next_on='%s'",
        stage, obs_source_get_name(source),
        obs_data_get_int(settings, "start_trigger"),
        obs_data_get_string(settings, "source"),
        obs_data_get_int(settings, "duration"),
        obs_data_get_int(settings, "duration_type"),
        obs_data_get_bool(settings, "custom_duration") ? 1 : 0,
        obs_data_get_bool(settings, "enabled_match_moving") ? 1 : 0,
        obs_data_get_int(settings, "easing_match"),
        obs_data_get_int(settings, "easing_function_match"),
        obs_data_get_string(settings, "simultaneous_move"),
        obs_data_get_string(settings, "next_move"),
        obs_data_get_string(settings, "next_move_on"));
    obs_data_release(settings);
}

workflow_filter_instance *workflow_filter_instance_create(
    obs_source_t *original, obs_source_t *parent, const workflow_node_t *node)
{
    if (!original || !parent || !node)
        return nullptr;
    workflow_filter_instance *result =
        (workflow_filter_instance *)calloc(1, sizeof(*result));
    if (!result)
        return nullptr;

    log_move_settings(original, "original before duplicate");
    char name[WORKFLOW_MAX_NAME];
    snprintf(name, sizeof(name), "%s [workflow:%p]",
             obs_source_get_name(original), (void *)result);
    result->instance = obs_source_duplicate(original, name, false);
    if (!result->instance) {
        free(result);
        return nullptr;
    }
    log_move_settings(result->instance, "duplicate before attach");

    result->original = obs_source_get_ref(original);
    result->parent = obs_source_get_ref(parent);
    obs_source_set_enabled(result->instance, false);
    obs_source_filter_add(parent, result->instance);
    log_move_settings(result->instance, "duplicate after attach");

    workflow_debug_log("Filter instance: duplicated '%s' -> '%s' node='%s'",
                       obs_source_get_name(original),
                       obs_source_get_name(result->instance), node->id);
    return result;
}

bool workflow_filter_instance_execute(workflow_filter_instance *instance)
{
    if (!instance || !instance->instance)
        return false;

    workflow_filter_diagnostics_log_runtime(instance->instance, "before execute");
    workflow_filter_diagnostics_log_target(instance->instance, "before execute");
    workflow_filter_diagnostics_begin(instance->instance, 1000);

    obs_source_set_enabled(instance->instance, false);

    deferred_enable *task =
        (deferred_enable *)calloc(1, sizeof(*task));
    if (!task)
        return false;

    task->filter = obs_source_get_ref(instance->instance);
    obs_queue_task(OBS_TASK_UI, enable_filter_task, task, false);

    workflow_filter_diagnostics_log_runtime(instance->instance, "after scheduling enable");
    workflow_filter_diagnostics_log_target(instance->instance, "after scheduling enable");
    workflow_debug_log("Filter instance: scheduled enable for temporary '%s'",
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
