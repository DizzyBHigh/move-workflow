#include "workflow-filter-instance.h"

#include "workflow-debug.h"

#include <obs.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void log_filter_state(const char *label, obs_source_t *source)
{
    if (!source)
        return;
    workflow_debug_log(
        "Filter diagnostic: %s name='%s' id='%s' enabled=%d active=%d showing=%d",
        label, obs_source_get_name(source), obs_source_get_unversioned_id(source),
        obs_source_enabled(source), obs_source_active(source), obs_source_showing(source));
}

static void log_move_source_state(const char *label, obs_source_t *source)
{
    if (!source || obs_source_get_unversioned_id(source) == nullptr ||
        strcmp(obs_source_get_unversioned_id(source), "move_source_filter") != 0)
        return;

    obs_data_t *settings = obs_source_get_settings(source);
    if (!settings)
        return;

    const char *target = obs_data_get_string(settings, "source");
    const char *transform = obs_data_get_string(settings, "transform_text");
    const int start_trigger = (int)obs_data_get_int(settings, "start_trigger");
    const int duration = (int)obs_data_get_int(settings, "duration");

    obs_data_t *pos = obs_data_get_obj(settings, "pos");
    const double x = pos ? obs_data_get_double(pos, "x") : 0.0;
    const double y = pos ? obs_data_get_double(pos, "y") : 0.0;
    if (pos)
        obs_data_release(pos);

    workflow_debug_log(
        "Move Source diagnostic: %s name='%s' target='%s' transform='%s' pos=(%.2f,%.2f) trigger=%d duration=%d",
        label, obs_source_get_name(source), target ? target : "", transform ? transform : "",
        x, y, start_trigger, duration);
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

    log_filter_state("original before duplicate", original);
    log_move_source_state("original before duplicate", original);

    char name[WORKFLOW_MAX_NAME];
    snprintf(name, sizeof(name), "%s [workflow:%p]",
             obs_source_get_name(original), (void *)result);
    result->instance = obs_source_duplicate(original, name, true);
    if (!result->instance) {
        free(result);
        return nullptr;
    }
    log_filter_state("runtime immediately after duplicate", result->instance);
    log_move_source_state("runtime immediately after duplicate", result->instance);

    result->original = obs_source_get_ref(original);
    result->parent = obs_source_get_ref(parent);
    obs_source_set_enabled(result->instance, false);
    obs_source_filter_add(parent, result->instance);

    workflow_debug_log("Filter instance: duplicated '%s' -> '%s' node='%s' parent='%s'",
                       obs_source_get_name(original), obs_source_get_name(result->instance),
                       node->id, obs_source_get_name(parent));
    log_filter_state("runtime after attach", result->instance);
    log_move_source_state("runtime after attach", result->instance);
    return result;
}

static void enable_source_on_ui(void *data)
{
    obs_source_t *source = (obs_source_t *)data;
    if (!source)
        return;

    log_filter_state("runtime UI enable before", source);
    log_move_source_state("runtime UI enable before", source);
    obs_source_set_enabled(source, true);
    log_filter_state("runtime UI enable after", source);
    log_move_source_state("runtime UI enable after", source);
    obs_source_release(source);
}

bool workflow_filter_instance_execute(workflow_filter_instance *instance)
{
    if (!instance || !instance->instance)
        return false;
    obs_source_t *source = obs_source_get_ref(instance->instance);
    if (!source)
        return false;

    log_filter_state("runtime execute", source);
    log_move_source_state("runtime execute", source);
    if (instance->original) {
        log_filter_state("original at execute", instance->original);
        log_move_source_state("original at execute", instance->original);
    }
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