#include "workflow-filter-settings.h"

#include "workflow-debug.h"

#include <obs-frontend-api.h>
#include <obs.h>
#include <cstring>

void workflow_filter_apply_node_settings(obs_source_t *filter,
                                         const workflow_node_t *node,
                                         uint64_t *workflow_duration_ms,
                                         uint64_t *restore_delay_ms)
{
    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings)
        return;

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

    obs_data_set_string(settings, "simultaneous_move", "");
    obs_data_set_string(settings, "next_move", "");
    obs_data_set_string(settings, "next_move_on", "move_end");
    if (node->start_trigger_mode == WORKFLOW_OVERRIDE &&
        strcmp(node->start_trigger_value, "Enable") == 0)
        obs_data_set_int(settings, "start_trigger", 5);
    obs_source_update(filter, settings);
    obs_data_release(settings);
    workflow_debug_log("Move dispatch: applied workflow execution overrides");
}
