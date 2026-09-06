#include "workflow-filter-instance.h"

#include "workflow-debug.h"

#include <cstdio>
#include <cstdlib>
#include <obs.h>

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
        "easing=%lld easing_function=%lld simultaneous='%s' next='%s' next_on='%s'",
        stage, obs_source_get_name(source),
        obs_data_get_int(settings, "start_trigger"),
        obs_data_get_string(settings, "source"),
        obs_data_get_int(settings, "duration"),
        obs_data_get_int(settings, "duration_type"),
        obs_data_get_bool(settings, "custom_duration") ? 1 : 0,
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
    result->instance = obs_source_duplicate(original, name, true);
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

static void log_runtime_state(obs_source_t *source, const char *stage)
{
    if (!source)
        return;
    obs_data_t *settings = obs_source_get_settings(source);
    const long long trigger = settings ? obs_data_get_int(settings, "start_trigger") : -1;
    const char *target = settings ? obs_data_get_string(settings, "source") : "";
    const bool enabled = obs_source_enabled(source);
    const bool active = obs_source_active(source);
    const bool showing = obs_source_showing(source);
    const char *id = obs_source_get_id(source);
    obs_source_t *parent = obs_filter_get_parent(source);
    const bool parent_active = parent ? obs_source_active(parent) : false;
    const bool parent_showing = parent ? obs_source_showing(parent) : false;
    const char *parent_name = parent ? obs_source_get_name(parent) : "";
    const bool target_exists = target && *target && obs_get_source_by_name(target) != nullptr;
    workflow_debug_log(
        "Filter instance: %s id='%s' enabled=%d active=%d showing=%d "
        "parent='%s' parent_active=%d parent_showing=%d start_trigger=%lld "
        "source='%s' target_exists=%d",
        stage, id ? id : "", enabled ? 1 : 0, active ? 1 : 0,
        showing ? 1 : 0, parent_name ? parent_name : "",
        parent_active ? 1 : 0, parent_showing ? 1 : 0, trigger,
        target ? target : "", target_exists ? 1 : 0);
    if (settings)
        obs_data_release(settings);
}

bool workflow_filter_instance_execute(workflow_filter_instance *instance)
{
    if (!instance || !instance->instance)
        return false;
    log_runtime_state(instance->instance, "before execute");
    obs_source_set_enabled(instance->instance, true);
    log_runtime_state(instance->instance, "after execute");
    workflow_debug_log("Filter instance: executing temporary '%s'",
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
