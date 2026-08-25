#include "workflow-node-timing-defaults.h"

#include <obs-frontend-api.h>
#include <obs.h>

#include <cstring>

namespace {
constexpr const char *kStartDelay = "start_delay";
constexpr const char *kCustomDuration = "custom_duration";
constexpr const char *kDuration = "duration";
constexpr const char *kEndDelay = "end_delay";

static bool supported_filter(const char *id)
{
    return id && (!std::strcmp(id, "move_action_filter") ||
                  !std::strcmp(id, "move_source_filter") ||
                  !std::strcmp(id, "move_source_swap_filter") ||
                  !std::strcmp(id, "move_value_filter"));
}
} // namespace

workflow_node_timing_defaults workflow_node_read_timing_defaults(
    const char *scene_name, const char *filter_name)
{
    workflow_node_timing_defaults result{};
    if (!scene_name || !scene_name[0] || !filter_name || !filter_name[0])
        return result;

    obs_source_t *scene = obs_get_source_by_name(scene_name);
    if (!scene)
        return result;
    obs_source_t *filter = obs_source_get_filter_by_name(scene, filter_name);
    obs_source_release(scene);
    if (!filter)
        return result;

    const char *id = obs_source_get_unversioned_id(filter);
    if (!supported_filter(id)) {
        obs_source_release(filter);
        return result;
    }

    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings) {
        obs_source_release(filter);
        return result;
    }

    result.start_delay_ms = (uint64_t)obs_data_get_int(settings, kStartDelay);
    result.end_delay_ms = (uint64_t)obs_data_get_int(settings, kEndDelay);
    if (obs_data_get_bool(settings, kCustomDuration))
        result.duration_ms = (uint64_t)obs_data_get_int(settings, kDuration);
    else
        result.duration_ms = (uint64_t)obs_frontend_get_transition_duration();
    result.valid = true;

    obs_data_release(settings);
    obs_source_release(filter);
    return result;
}
