#include "workflow-runtime.h"
#include "workflow-shortcuts.h"
#include "workflow-debug.h"
#include <obs.h>
#include <obs-module.h>
#include <stdlib.h>
#include <string.h>
#include <util/platform.h>
#include <util/threading.h>

#define PHASE12_TEST_DURATION_MS 1000
#define MOVE_START_TRIGGER_ENABLE 5

typedef struct duration_restore_context {
    obs_source_t *filter;
    bool custom_duration;
    long long duration;
    long long start_trigger;
} duration_restore_context_t;

typedef struct enable_filter_context {
    obs_source_t *filter;
    workflow_t *workflow;
    char node_id[WORKFLOW_MAX_NAME];
} enable_filter_context_t;

static obs_source_t *find_move_filter(const workflow_action_ref_t *action)
{
    const char *expected_id = workflow_expected_filter_id(action->kind);
    workflow_debug_log("Runtime lookup: scene='%s' filter='%s' id='%s' expected='%s'",
                       action->scene_name, action->filter_name, action->filter_id,
                       expected_id ? expected_id : "(null)");
    if (!expected_id || !strlen(expected_id)) {
        workflow_debug_log("Runtime lookup FAILED: unsupported move kind");
        return NULL;
    }
    if (action->filter_id[0] && strcmp(action->filter_id, expected_id) != 0) {
        workflow_debug_log("Runtime lookup FAILED: configured filter id mismatch");
        return NULL;
    }
    obs_source_t *scene = obs_get_source_by_name(action->scene_name);
    if (!scene) {
        workflow_debug_log("Runtime lookup FAILED: scene not found '%s'", action->scene_name);
        return NULL;
    }
    obs_source_t *filter = obs_source_get_filter_by_name(scene, action->filter_name);
    obs_source_release(scene);
    if (!filter) {
        workflow_debug_log("Runtime lookup FAILED: filter not found '%s'", action->filter_name);
        return NULL;
    }
    const char *actual_id = obs_source_get_id(filter);
    if (!actual_id || strcmp(actual_id, expected_id) != 0) {
        workflow_debug_log("Runtime lookup FAILED: actual filter id mismatch");
        obs_source_release(filter);
        return NULL;
    }
    workflow_debug_log("Runtime lookup SUCCESS: target filter resolved");
    return filter;
}

static workflow_node_t *find_node(workflow_t *wf, const char *id)
{
    if (!wf || !id) return NULL;
    for (size_t i = 0; i < wf->node_count; ++i)
        if (strcmp(wf->nodes[i].id, id) == 0) return &wf->nodes[i];
    return NULL;
}

static void restore_filter_settings(void *data)
{
    duration_restore_context_t *ctx = data;
    if (!ctx) return;
    obs_data_t *settings = obs_source_get_settings(ctx->filter);
    if (settings) {
        obs_data_set_bool(settings, "custom_duration", ctx->custom_duration);
        obs_data_set_int(settings, "duration", ctx->duration);
        obs_data_set_int(settings, "start_trigger", ctx->start_trigger);
        obs_source_update(ctx->filter, settings);
        obs_data_release(settings);
    }
    obs_source_release(ctx->filter);
    free(ctx);
}

static void enable_filter_task(void *data)
{
    enable_filter_context_t *ctx = data;
    if (!ctx) return;
    obs_source_set_enabled(ctx->filter, true);
    workflow_debug_log("Runtime action: queued enable applied for node='%s'", ctx->node_id);
    workflow_shortcuts_begin(ctx->workflow, find_node(ctx->workflow, ctx->node_id));
    obs_source_release(ctx->filter);
    free(ctx);
}

static void *restore_filter_thread(void *data)
{
    duration_restore_context_t *ctx = data;
    os_set_thread_name("move-workflow-filter-restore");
    os_sleep_ms(PHASE12_TEST_DURATION_MS + 150);
    obs_queue_task(OBS_TASK_UI, restore_filter_settings, ctx, false);
    return NULL;
}

static bool apply_overrides(obs_source_t *filter, const workflow_node_t *node)
{
    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings) return false;
    duration_restore_context_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) { obs_data_release(settings); return false; }
    ctx->filter = obs_source_get_ref(filter);
    ctx->custom_duration = obs_data_get_bool(settings, "custom_duration");
    ctx->duration = obs_data_get_int(settings, "duration");
    ctx->start_trigger = obs_data_get_int(settings, "start_trigger");
    if (node->duration.mode == WORKFLOW_OVERRIDE) {
        obs_data_set_bool(settings, "custom_duration", true);
        obs_data_set_int(settings, "duration", (long long)node->duration.duration_ms);
    }
    if (node->start_trigger_mode == WORKFLOW_OVERRIDE &&
        strcmp(node->start_trigger_value, "Enable") == 0)
        obs_data_set_int(settings, "start_trigger", MOVE_START_TRIGGER_ENABLE);
    obs_source_update(filter, settings);
    obs_data_release(settings);
    pthread_t thread;
    if (pthread_create(&thread, NULL, restore_filter_thread, ctx) != 0) {
        restore_filter_settings(ctx);
        return false;
    }
    pthread_detach(thread);
    return true;
}

static bool queue_filter_enable(obs_source_t *filter, workflow_t *workflow,
                                const workflow_node_t *node)
{
    enable_filter_context_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return false;
    ctx->filter = obs_source_get_ref(filter);
    ctx->workflow = workflow;
    strncpy(ctx->node_id, node->id, sizeof(ctx->node_id) - 1);
    obs_source_set_enabled(filter, false);
    workflow_debug_log("Runtime action: filter disabled; queueing enable for node='%s'",
                       node->id);
    obs_queue_task(OBS_TASK_UI, enable_filter_task, ctx, false);
    return true;
}

static bool execute_node(workflow_t *workflow, workflow_node_t *node)
{
    if (!workflow || !node) return false;
    workflow_debug_log("Runtime execute: node='%s' type=%d", node->id, (int)node->type);
    obs_source_t *filter = find_move_filter(&node->action);
    if (!filter) {
        workflow_debug_log("Runtime execute FAILED: target could not be resolved");
        return false;
    }
    if (!apply_overrides(filter, node)) {
        workflow_debug_log("Runtime execute FAILED: filter settings override");
        obs_source_release(filter);
        return false;
    }
    if (!queue_filter_enable(filter, workflow, node)) {
        workflow_debug_log("Runtime execute FAILED: could not queue filter enable");
        obs_source_release(filter);
        return false;
    }
    obs_source_release(filter);
    workflow_debug_log("Runtime execute SUCCESS: node='%s' enable queued", node->id);
    return true;
}

bool workflow_runtime_execute_node_by_id(workflow_t *workflow, const char *node_id)
{
    if (!workflow || !workflow->enabled) return false;
    workflow_node_t *node = find_node(workflow, node_id);
    if (!node) {
        workflow_debug_log("Runtime execute FAILED: node not found '%s'", node_id ? node_id : "(null)");
        return false;
    }
    return execute_node(workflow, node);
}

void workflow_runtime_test_duration(workflow_t *workflow)
{
    workflow_node_t *node = find_node(workflow, "move-left");
    if (!node) return;
    node->duration.mode = WORKFLOW_OVERRIDE;
    node->duration.duration_ms = PHASE12_TEST_DURATION_MS;
    execute_node(workflow, node);
    node->duration.mode = WORKFLOW_USE_EXISTING;
}
