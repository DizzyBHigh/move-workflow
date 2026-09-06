#include "workflow-filter-instance-helpers.hpp"

#include "workflow-debug.h"

#include <cstdio>
#include <cstring>

bool workflow_filter_instance_rebind_move_source(obs_source_t *filter)
{
    if (!filter || strcmp(obs_source_get_id(filter), "move_source_filter") != 0)
        return false;

    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings)
        return false;

    const char *source = obs_data_get_string(settings, "source");
    if (!source || !*source) {
        obs_data_release(settings);
        return false;
    }

    char source_name[1024];
    snprintf(source_name, sizeof(source_name), "%s", source);
    obs_data_set_string(settings, "source", "");
    obs_source_update(filter, settings);
    obs_data_set_string(settings, "source", source_name);
    obs_source_update(filter, settings);
    obs_data_release(settings);
    return true;
}

bool workflow_filter_instance_start_native(obs_source_t *filter)
{
    if (!filter)
        return false;

    const char *id = obs_source_get_unversioned_id(filter);
    const char *property_name = nullptr;
    if (id && strcmp(id, "move_source_filter") == 0)
        property_name = "move_source_start";
    else if (id && strcmp(id, "move_source_swap_filter") == 0)
        property_name = "move_source_start";
    else if (id && strcmp(id, "move_value_filter") == 0)
        property_name = "move_value_start";
    else if (id && strcmp(id, "move_action_filter") == 0)
        property_name = "move_filter_start";

    if (!property_name) {
        workflow_debug_log("Filter instance: no native Start property id='%s'",
                           id ? id : "");
        return false;
    }

    workflow_debug_log(
        "Filter instance: native Start prep id='%s' enabled_before=%d source='%s'",
        id, obs_source_enabled(filter) ? 1 : 0,
        [&]() {
            obs_data_t *settings = obs_source_get_settings(filter);
            const char *source = settings ? obs_data_get_string(settings, "source") : "";
            static char value[1024];
            snprintf(value, sizeof(value), "%s", source ? source : "");
            if (settings)
                obs_data_release(settings);
            return value;
        }());

    if (!obs_source_enabled(filter))
        obs_source_set_enabled(filter, true);

    obs_properties_t *props = obs_source_properties(filter);
    if (!props) {
        workflow_debug_log("Filter instance: native Start properties unavailable id='%s'", id);
        return false;
    }

    obs_property_t *property = obs_properties_get(props, property_name);
    workflow_debug_log(
        "Filter instance: native Start property id='%s' property='%s' found=%d enabled_after=%d",
        id, property_name, property ? 1 : 0, obs_source_enabled(filter) ? 1 : 0);

    if (!property) {
        obs_properties_destroy(props);
        return false;
    }

    /* Exeldro's Start callbacks return false even when they successfully
       invoke their internal start routine, so this return value is not a
       reliable indication of whether the action started. */
    obs_property_button_clicked(property, filter);
    workflow_debug_log("Filter instance: native Start callback invoked id='%s' property='%s'",
                       id, property_name);
    obs_properties_destroy(props);
    return true;
}
