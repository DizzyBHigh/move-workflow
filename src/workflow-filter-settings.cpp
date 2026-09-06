#include "workflow-filter-settings.h"

#include "workflow-debug.h"

#include <obs-frontend-api.h>
#include <obs.h>
#include <cstring>

static void log_source_name(obs_source_t *filter, const char *stage)
{
    if (!filter)
        return;

    obs_data_t *settings = obs_source_get_settings(filter);
    const char *source_name = settings ? obs_data_get_string(settings, "source_name") : "";
    workflow_debug_log("Move dispatch: %s filter='%s' source_name='%s'",
                       stage,
                       obs_source_get_name(filter),
                       source_name ? source_name : "");
    if (settings)
        obs_data_release(settings);
}

void workflow_filter_apply_node_settings(obs_source_t *filter,
                                         const workflow_node_t *node,
                                         uint64_t *workflow_duration_ms,
                                         uint64_t *restore_delay_ms)
{
    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings)
        return;

    const char *source_name = obs_data_get_string(settings, "source_name");
    workflow_debug_log("Move dispatch: before overrides filter='%s' source_name='%s'",
                       obs_source_get_name(filter),
                       source_name ? source_name : "");

    const uint64_t native_duration = (uint64_t)obs_data_get_int(settings, "duration");
    *workflow_duration_ms = native_duration;
    *restore_delay_ms = native_duration;

    if (node->action.kind == WORKFLOW_MOVE_ACTION) {
        const int duration_type = (int)obs_data_get_int(settings, "duration_type");
        if (duration_type == 1)
            *restore_delay_ms = (uint64_t)obs_frontend_get_transition_duration();
        else if (duration_type == 2)
            *restore_delay_ms = 0;
    }

    if (node->duration.mode == WORKFLOW_OVERRIDE) {
        *workflow_duration_ms = node->duration.duration_ms;
        if (node->action.kind != WORKFLOW_MOVE_ACTION) {
            *restore_delay_ms = *workflow_duration_ms;
            obs_data_set_bool(settings, "custom_duration", true);
            obs_data_set_int(settings, "duration", (long long)*workflow_duration_ms);
        } else if (*restore_delay_ms < *workflow_duration_ms) {
            *restore_delay_ms = *workflow_duration_ms;
        }
    }

    if (node->easing.mode == WORKFLOW_OVERRIDE) {
        obs_data_set_int(settings, "easing_match", node->easing.easing);
        obs_data_set_int(settings, "easing_function_match", node->easing.function);
    }

    obs_data_set_string(settings, "simultaneous_move", "");
    obs_data_set_string(settings, "next_move", "");
    obs_data_set_string(settings, "next_move_on", "move_end");
    obs_data_set_int(settings, "start_trigger", 5);

    workflow_debug_log("Move dispatch: before obs_source_update filter='%s' source_name='%s'",
                       obs_source_get_name(filter),
                       obs_data_get_string(settings, "source_name"));
    obs_source_update(filter, settings);
    obs_data_release(settings);

    log_source_name(filter, "after obs_source_update");
    workflow_debug_log("Move dispatch: applied workflow execution overrides");
}
