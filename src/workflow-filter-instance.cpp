#include "workflow-filter-instance.h"

#include "workflow-debug.h"
#include "workflow-engine-delay.h"

#include <cstdlib>
#include <cstring>

static void apply_node_settings(obs_source_t *filter, const workflow_node_t *node,
                                uint64_t *duration_ms)
{
    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings)
        return;
    *duration_ms = (uint64_t)obs_data_get_int(settings, "duration");
    if (node->duration.mode == WORKFLOW_OVERRIDE) {
        *duration_ms = node->duration.duration_ms;
        obs_data_set_bool(settings, "custom_duration", true);
        obs_data_set_int(settings, "duration", (long long)*duration_ms);
    }
    obs_data_set_string(settings, "simultaneous_move", "");
    obs_data_set_string(settings, "next_move", "");
    obs_data_set_string(settings, "next_move_on", "move_end");
    obs_data_set_int(settings, "start_trigger", 5);
    obs_source_update(filter, settings);
    obs_data_release(settings);
    workflow_debug_log("Move dispatch: using original filter with native ENABLE trigger");
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
    result->original = obs_source_get_ref(original);
    result->parent = obs_source_get_ref(parent);
    result->instance = obs_source_get_ref(original);
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
    return true;
}

void workflow_filter_instance_destroy(workflow_filter_instance *instance)
{
    if (!instance)
        return;
    if (instance->instance)
        obs_source_release(instance->instance);
    if (instance->parent)
        obs_source_release(instance->parent);
    if (instance->original)
        obs_source_release(instance->original);
    free(instance);
}

typedef struct destroy_context {
    workflow_filter_instance *instance;
    obs_data_t *restore_settings;
} destroy_context;

static void destroy_on_ui(void *data)
{
    destroy_context *ctx = (destroy_context *)data;
    if (!ctx)
        return;
    workflow_debug_log("Filter instance: restoring native Move filter");
    if (ctx->instance && ctx->instance->instance) {
        obs_source_set_enabled(ctx->instance->instance, false);
        if (ctx->restore_settings)
            obs_source_update(ctx->instance->instance, ctx->restore_settings);
    }
    if (ctx->restore_settings)
        obs_data_release(ctx->restore_settings);
    workflow_filter_instance_destroy(ctx->instance);
    free(ctx);
}

static void schedule_destroy(workflow_filter_instance *instance,
                             obs_data_t *restore_settings, uint64_t duration_ms)
{
    destroy_context *ctx = (destroy_context *)calloc(1, sizeof(*ctx));
    if (!ctx) {
        obs_data_release(restore_settings);
        workflow_filter_instance_destroy(instance);
        return;
    }
    ctx->instance = instance;
    ctx->restore_settings = restore_settings;
    const uint64_t delay_ms = duration_ms ? duration_ms + 25 : 25;
    if (!workflow_engine_delay_start(delay_ms, destroy_on_ui, ctx)) {
        obs_data_release(ctx->restore_settings);
        workflow_filter_instance_destroy(ctx->instance);
        free(ctx);
    }
}

bool workflow_filter_instance_execute_node(workflow_t *workflow, workflow_node_t *node)
{
    if (!workflow || !node || !workflow->enabled)
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
    if (!expected || !actual || strcmp(expected, actual) != 0) {
        obs_source_release(original);
        obs_source_release(parent);
        return false;
    }
    obs_data_t *restore_settings = obs_source_get_settings(original);
    workflow_filter_instance *instance =
        workflow_filter_instance_create(original, parent, node);
    obs_source_release(original);
    obs_source_release(parent);
    if (!instance) {
        if (restore_settings)
            obs_data_release(restore_settings);
        return false;
    }
    uint64_t duration_ms = 0;
    apply_node_settings(instance->instance, node, &duration_ms);
    if (!workflow_filter_instance_execute(instance)) {
        if (restore_settings)
            obs_data_release(restore_settings);
        workflow_filter_instance_destroy(instance);
        return false;
    }
    schedule_destroy(instance, restore_settings, duration_ms);
    return true;
}
