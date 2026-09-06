#include "workflow-filter-activation.h"

#include "workflow-debug.h"

#include <cstring>

bool workflow_filter_restore(obs_source_t *runtime, obs_source_t *original)
{
    if (!runtime || !original)
        return false;
    obs_data_t *settings = obs_source_get_settings(original);
    if (!settings)
        return false;
    obs_source_update(runtime, settings);
    obs_data_release(settings);
    return true;
}

bool workflow_filter_activate(obs_source_t *source, obs_source_t *parent)
{
    if (!source)
        return false;

    const char *id = obs_source_get_unversioned_id(source);
    if (!id || strcmp(id, "move_source_filter") != 0) {
        obs_source_set_enabled(source, true);
        return true;
    }

    obs_data_t *settings = obs_source_get_settings(source);
    if (!settings)
        return false;
    const bool match_moving = obs_data_get_bool(settings, "enabled_match_moving");
    obs_data_set_int(settings, "start_trigger", 5);
    obs_data_set_bool(settings, "enabled_match_moving", true);
    obs_source_set_enabled(source, true);
    obs_source_update(source, settings);
    obs_data_set_bool(settings, "enabled_match_moving", match_moving);
    obs_source_update(source, settings);
    obs_data_release(settings);
    workflow_debug_log("Filter activation: Move filter '%s' explicitly retriggered", obs_source_get_name(source));
    (void)parent;
    return true;
}
