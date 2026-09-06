#include "workflow-filter-activation.h"

#include "workflow-debug.h"

#include <cstring>

bool workflow_filter_activate(obs_source_t *source, obs_source_t *parent)
{
    if (!source)
        return false;

    const char *id = obs_source_get_unversioned_id(source);
    if (!id || strcmp(id, "move_source_filter") != 0) {
        obs_source_set_enabled(source, true);
        return true;
    }

    obs_source_set_enabled(source, false);

    obs_data_t *settings = obs_source_get_settings(source);
    if (settings) {
        obs_data_set_int(settings, "start_trigger", 5);
        obs_source_update(source, settings);
        obs_data_release(settings);
    }

    obs_source_set_enabled(source, true);

    settings = obs_source_get_settings(source);
    if (settings) {
        obs_data_set_int(settings, "start_trigger", 5);
        obs_source_update(source, settings);
        obs_data_release(settings);
    }

    workflow_debug_log("Filter activation: Move filter '%s' retriggered", obs_source_get_name(source));
    (void)parent;
    return true;
}
