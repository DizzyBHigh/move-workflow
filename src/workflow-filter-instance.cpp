#include "workflow-filter-instance.h"

#include "workflow-debug.h"
#include "workflow-filter-activation.h"

#include <obs.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void log_filter_state(const char *label, obs_source_t *source)
{
    if (!source) return;
    workflow_debug_log("Filter diagnostic: %s name='%s' id='%s' enabled=%d active=%d showing=%d",
                       label, obs_source_get_name(source), obs_source_get_unversioned_id(source),
                       obs_source_enabled(source), obs_source_active(source), obs_source_showing(source));
}

static void log_move_source_state(const char *label, obs_source_t *source)
{
    if (!source || !obs_source_get_unversioned_id(source) ||
        strcmp(obs_source_get_unversioned_id(source), "move_source_filter") != 0) return;
    obs_data_t *settings = obs_source_get_settings(source);
    if (!settings) return;
    const char *target = obs_data_get_string(settings, "source");
    const char *transform = obs_data_get_string(settings, "transform_text");
    const char *simultaneous = obs_data_get_string(settings, "simultaneous_move");
    const char *next = obs_data_get_string(settings, "next_move");
    const char *next_on = obs_data_get_string(settings, "next_move_on");
    const int trigger = (int)obs_data_get_int(settings, "start_trigger");
    const int duration = (int)obs_data_get_int(settings, "duration");
    const int duration_type = (int)obs_data_get_int(settings, "duration_type");
    const int custom_duration = obs_data_get_bool(settings, "custom_duration");
    const int easing = (int)obs_data_get_int(settings, "easing_match");
    const int easing_function = (int)obs_data_get_int(settings, "easing_function_match");
    obs_data_t *pos = obs_data_get_obj(settings, "pos");
    const double x = pos ? obs_data_get_double(pos, "x") : 0.0;
    const double y = pos ? obs_data_get_double(pos, "y") : 0.0;
    if (pos) obs_data_release(pos);
    workflow_debug_log("Move Source diagnostic: %s name='%s' target='%s' transform='%s' pos=(%.2f,%.2f) trigger=%d duration=%d duration_type=%d custom_duration=%d easing=%d easing_function=%d simultaneous='%s' next='%s' next_on='%s'",
                       label, obs_source_get_name(source), target ? target : "", transform ? transform : "", x, y,
                       trigger, duration, duration_type, custom_duration, easing, easing_function,
                       simultaneous ? simultaneous : "", next ? next : "", next_on ? next_on : "");
    obs_data_release(settings);
}

static void log_scene_item_state(const char *label, obs_source_t *parent, obs_source_t *filter)
{
    if (!parent || !filter) return;
    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings) return;
    const char *target = obs_data_get_string(settings, "source");
    obs_scene_t *scene = obs_scene_from_source(parent);
    obs_sceneitem_t *item = scene && target ? obs_scene_find_source(scene, target) : nullptr;
    if (!item) {
        workflow_debug_log("Scene item diagnostic: %s scene='%s' target='%s' item=NOT_FOUND",
                           label, obs_source_get_name(parent), target ? target : "");
        obs_data_release(settings);
        return;
    }
    struct vec2 pos, scale;
    obs_sceneitem_get_pos(item, &pos);
    obs_sceneitem_get_scale(item, &scale);
    workflow_debug_log("Scene item diagnostic: %s scene='%s' target='%s' pos=(%.2f,%.2f) scale=(%.4f,%.4f) rot=%.2f",
                       label, obs_source_get_name(parent), target, pos.x, pos.y, scale.x, scale.y, obs_sceneitem_get_rot(item));
    obs_data_release(settings);
}

workflow_filter_instance *workflow_filter_instance_create(obs_source_t *original, obs_source_t *parent,
                                                           const workflow_node_t *node)
{
    if (!original || !parent || !node) return nullptr;
    workflow_filter_instance *result = (workflow_filter_instance *)calloc(1, sizeof(*result));
    if (!result) return nullptr;
    log_filter_state("original before duplicate", original);
    log_move_source_state("original before duplicate", original);
    char name[WORKFLOW_MAX_NAME];
    snprintf(name, sizeof(name), "%s [workflow:%p]", obs_source_get_name(original), (void *)result);
    result->instance = obs_source_duplicate(original, name, true);
    if (!result->instance) { free(result); return nullptr; }
    log_filter_state("runtime immediately after duplicate", result->instance);
    log_move_source_state("runtime immediately after duplicate", result->instance);
    result->original = obs_source_get_ref(original);
    result->parent = obs_source_get_ref(parent);
    obs_source_set_enabled(result->instance, false);
    obs_source_filter_add(parent, result->instance);
    workflow_debug_log("Filter instance: duplicated '%s' -> '%s' node='%s' parent='%s'",
                       obs_source_get_name(original), obs_source_get_name(result->instance), node->id, obs_source_get_name(parent));
    log_filter_state("runtime after attach", result->instance);
    log_move_source_state("runtime after attach", result->instance);
    log_scene_item_state("runtime after attach", parent, result->instance);
    return result;
}

struct enable_task_data { obs_source_t *source; obs_source_t *parent; };

static void sample_source_after_enable(void *data)
{
    enable_task_data *task = (enable_task_data *)data;
    if (!task) return;
    log_filter_state("runtime post-enable sample", task->source);
    log_move_source_state("runtime post-enable sample", task->source);
    log_scene_item_state("runtime post-enable sample", task->parent, task->source);
    if (task->source) obs_source_release(task->source);
    if (task->parent) obs_source_release(task->parent);
    free(task);
}

static void enable_source_on_ui(void *data)
{
    enable_task_data *task = (enable_task_data *)data;
    if (!task) return;
    obs_source_t *source = task->source, *parent = task->parent;
    if (source) {
        log_filter_state("runtime UI enable before", source);
        log_move_source_state("runtime UI enable before", source);
        log_scene_item_state("runtime UI enable before", parent, source);
        obs_source_set_enabled(source, true);
        obs_data_t *settings = obs_source_get_settings(source);
        if (settings) {
            obs_data_set_int(settings, "start_trigger", 5);
            obs_source_update(source, settings);
            obs_data_release(settings);
        }
        log_filter_state("runtime UI enable after", source);
        log_move_source_state("runtime UI enable after", source);
        log_scene_item_state("runtime UI enable after", parent, source);
        obs_queue_task(OBS_TASK_UI, sample_source_after_enable, task, false);
        return;
    }
    if (parent) obs_source_release(parent);
    free(task);
}

bool workflow_filter_instance_execute(workflow_filter_instance *instance)
{
    if (!instance || !instance->instance) return false;
    obs_source_t *source = obs_source_get_ref(instance->instance);
    if (!source) return false;
    workflow_filter_prepare_for_execution(source, instance->original, instance->node);
    log_filter_state("runtime execute", source);
    log_move_source_state("runtime execute", source);
    log_scene_item_state("runtime execute", instance->parent, source);
    if (instance->original) {
        log_filter_state("original at execute", instance->original);
        log_move_source_state("original at execute", instance->original);
    }
    enable_task_data *task = (enable_task_data *)calloc(1, sizeof(*task));
    if (!task) { obs_source_release(source); return false; }
    task->source = source;
    task->parent = instance->parent ? obs_source_get_ref(instance->parent) : nullptr;
    obs_queue_task(OBS_TASK_UI, enable_source_on_ui, task, false);
    workflow_debug_log("Filter instance: queued enable '%s'", obs_source_get_name(instance->instance));
    return true;
}

void workflow_filter_instance_destroy(workflow_filter_instance *instance)
{
    if (!instance) return;
    if (instance->parent && instance->instance) obs_source_filter_remove(instance->parent, instance->instance);
    if (instance->instance) obs_source_release(instance->instance);
    if (instance->parent) obs_source_release(instance->parent);
    if (instance->original) obs_source_release(instance->original);
    free(instance);
}
