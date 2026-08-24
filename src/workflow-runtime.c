#include "workflow-runtime.h"
#include "workflow-shortcuts.h"

#include <obs.h>
#include <obs-module.h>
#include <stdlib.h>
#include <string.h>
#include <util/platform.h>
#include <util/threading.h>

#define PHASE12_TEST_DURATION_MS 1000

typedef struct duration_restore_context {
    obs_source_t *filter;
    bool custom_duration;
    long long duration;
} duration_restore_context_t;

static obs_source_t *find_move_filter(const workflow_action_ref_t *action)
{
    const char *expected_id = workflow_expected_filter_id(action->kind);
    if (!expected_id || !strlen(expected_id) || strcmp(action->filter_id, expected_id) != 0)
        return NULL;
    obs_source_t *scene = obs_get_source_by_name(action->scene_name);
    if (!scene)
        return NULL;
    obs_source_t *filter = obs_source_get_filter_by_name(scene, action->filter_name);
    obs_source_release(scene);
    if (!filter || strcmp(obs_source_get_id(filter), action->filter_id) != 0) {
        if (filter)
            obs_source_release(filter);
        return NULL;
    }
    return filter;
}

static workflow_node_t *find_node(workflow_t *wf, const char *id)
{
    if (!wf || !id)
        return NULL;
    for (size_t i = 0; i < wf->node_count; ++i)
        if (strcmp(wf->nodes[i].id, id) == 0)
            return &wf->nodes[i];
    return NULL;
}

static void restore_duration(void *data)
{
    duration_restore_context_t *ctx = data;
    if (!ctx)
        return;
    obs_data_t *settings = obs_source_get_settings(ctx->filter);
    if (settings) {
        obs_data_set_bool(settings, "custom_duration", ctx->custom_duration);
        obs_data_set_int(settings, "duration", ctx->duration);
        obs_source_update(ctx->filter, settings);
        obs_data_release(settings);
    }
    obs_source_release(ctx->filter);
    free(ctx);
}

static void *duration_restore_thread(void *data)
{
    duration_restore_context_t *ctx = data;
    os_set_thread_name("move-workflow-duration-restore");
    os_sleep_ms(PHASE12_TEST_DURATION_MS + 150);
    obs_queue_task(OBS_TASK_UI, restore_duration, ctx, false);
    return NULL;
}

static bool apply_duration_override(obs_source_t *filter, uint64_t duration_ms)
{
    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings)
        return false;
    duration_restore_context_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        obs_data_release(settings);
        return false;
    }
    ctx->filter = obs_source_get_ref(filter);
    ctx->custom_duration = obs_data_get_bool(settings, "custom_duration");
    ctx->duration = obs_data_get_int(settings, "duration");
    obs_data_set_bool(settings, "custom_duration", true);
    obs_data_set_int(settings, "duration", (long long)duration_ms);
    obs_source_update(filter, settings);
    obs_data_release(settings);
    pthread_t thread;
    if (pthread_create(&thread, NULL, duration_restore_thread, ctx) != 0) {
        restore_duration(ctx);
        return false;
    }
    pthread_detach(thread);
    return true;
}

static void apply_start_trigger_override(obs_source_t *filter, const workflow_node_t *node)
{
    if (node->start_trigger_mode == WORKFLOW_OVERRIDE &&
        strcmp(node->start_trigger_value, "Enable") == 0) {
        obs_source_set_enabled(filter, false);
        obs_source_set_enabled(filter, true);
        return;
    }
    obs_source_set_enabled(filter, false);
    obs_source_set_enabled(filter, true);
}

static void execute_node(workflow_t *workflow, workflow_node_t *node)
{
    if (!workflow || !node)
        return;
    obs_source_t *filter = find_move_filter(&node->action);
    if (!filter)
        return;
    if (node->duration.mode == WORKFLOW_OVERRIDE &&
        !apply_duration_override(filter, node->duration.duration_ms)) {
        obs_source_release(filter);
        return;
    }
    apply_start_trigger_override(filter, node);
    obs_source_release(filter);
    workflow_shortcuts_begin(workflow, node);
}

void workflow_runtime_execute_node_by_id(workflow_t *workflow, const char *node_id)
{
    if (!workflow || !workflow->enabled)
        return;
    workflow_node_t *node = find_node(workflow, node_id);
    if (node)
        execute_node(workflow, node);
}

void workflow_runtime_test_duration(workflow_t *workflow)
{
    workflow_node_t *node = find_node(workflow, "move-left");
    if (!node)
        return;
    node->duration.mode = WORKFLOW_OVERRIDE;
    node->duration.duration_ms = PHASE12_TEST_DURATION_MS;
    execute_node(workflow, node);
    node->duration.mode = WORKFLOW_USE_EXISTING;
}
