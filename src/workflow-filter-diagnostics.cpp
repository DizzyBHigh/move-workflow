#include "workflow-filter-diagnostics.hpp"

#include "workflow-debug.h"

#include <obs.h>
#include <obs-frontend-api.h>

#include <cstdio>
#include <cstdlib>

namespace {
struct deferred_probe {
    obs_source_t *filter = nullptr;
    uint32_t remaining = 0;
};

static void log_target(obs_source_t *filter, const char *stage)
{
    obs_source_t *parent = obs_filter_get_parent(filter);
    obs_data_t *settings = obs_source_get_settings(filter);
    const char *target = settings ? obs_data_get_string(settings, "source") : "";
    obs_scene_t *scene = obs_scene_from_source(parent);
    obs_sceneitem_t *item = scene && target && *target
        ? obs_scene_find_source(scene, target) : nullptr;
    float x = 0.0f;
    float y = 0.0f;

    if (item) {
        struct obs_transform_info info = {};
        obs_sceneitem_get_info2(item, &info);
        x = info.pos.x;
        y = info.pos.y;
    }

    workflow_debug_log(
        "Filter tick target: %s filter='%s' target='%s' pos=(%.3f,%.3f)",
        stage, obs_source_get_name(filter), target ? target : "", x, y);

    if (settings)
        obs_data_release(settings);
}

static void deferred_probe_task(void *param)
{
    auto *probe = static_cast<deferred_probe *>(param);
    if (!probe || !probe->filter)
        return;

    workflow_filter_diagnostics_log_runtime(probe->filter, "deferred graphics task");
    log_target(probe->filter, "deferred graphics task");

    if (--probe->remaining) {
        obs_queue_task(OBS_TASK_GRAPHICS, deferred_probe_task, probe, false);
        return;
    }

    obs_source_release(probe->filter);
    free(probe);
}
}

void workflow_filter_diagnostics_log_runtime(obs_source_t *source,
                                             const char *stage)
{
    if (!source)
        return;

    obs_source_t *parent = obs_filter_get_parent(source);
    obs_data_t *settings = obs_source_get_settings(source);
    const char *target = settings ? obs_data_get_string(settings, "source") : "";
    const long long trigger = settings ? obs_data_get_int(settings, "start_trigger") : -1;

    workflow_debug_log(
        "Filter tick probe: %s id='%s' enabled=%d active=%d showing=%d "
        "parent='%s' parent_active=%d parent_showing=%d start_trigger=%lld "
        "source='%s'",
        stage ? stage : "state", obs_source_get_id(source),
        obs_source_enabled(source) ? 1 : 0,
        obs_source_active(source) ? 1 : 0,
        obs_source_showing(source) ? 1 : 0,
        parent ? obs_source_get_name(parent) : "",
        parent && obs_source_active(parent) ? 1 : 0,
        parent && obs_source_showing(parent) ? 1 : 0,
        trigger, target ? target : "");

    if (settings)
        obs_data_release(settings);
}

void workflow_filter_diagnostics_log_target(obs_source_t *filter,
                                            const char *stage)
{
    if (!filter)
        return;

    obs_source_t *parent = obs_filter_get_parent(filter);
    if (!parent)
        return;

    obs_data_t *settings = obs_source_get_settings(filter);
    const char *target = settings ? obs_data_get_string(settings, "source") : "";
    obs_scene_t *scene = obs_scene_from_source(parent);
    obs_sceneitem_t *item = scene && target && *target
        ? obs_scene_find_source(scene, target) : nullptr;

    if (item) {
        struct obs_transform_info info = {};
        obs_sceneitem_get_info2(item, &info);
        workflow_debug_log(
            "Filter tick target: %s filter='%s' target='%s' "
            "pos=(%.3f,%.3f) scale=(%.3f,%.3f) rot=%.3f",
            stage ? stage : "state", obs_source_get_name(filter),
            target, info.pos.x, info.pos.y, info.scale.x, info.scale.y,
            info.rot);
    }

    if (settings)
        obs_data_release(settings);
}

void workflow_filter_diagnostics_begin(obs_source_t *filter,
                                       uint32_t duration_ms)
{
    if (!filter)
        return;

    workflow_debug_log(
        "Filter tick probe: begin filter='%s' duration=%u frontend_active=%d",
        obs_source_get_name(filter), duration_ms,
        obs_frontend_streaming_active() || obs_frontend_recording_active() ? 1 : 0);

    auto *probe = static_cast<deferred_probe *>(calloc(1, sizeof(*probe)));
    if (!probe)
        return;

    probe->filter = obs_source_get_ref(filter);
    probe->remaining = 3;
    obs_queue_task(OBS_TASK_GRAPHICS, deferred_probe_task, probe, false);
}
