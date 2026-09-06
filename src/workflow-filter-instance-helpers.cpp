#include "workflow-filter-instance-helpers.hpp"

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

    if (!property_name)
        return false;

    obs_properties_t *props = obs_source_properties(filter);
    if (!props)
        return false;
    obs_property_t *property = obs_properties_get(props, property_name);
    const bool started = property && obs_property_button_clicked(property, filter);
    obs_properties_destroy(props);
    return started;
}
