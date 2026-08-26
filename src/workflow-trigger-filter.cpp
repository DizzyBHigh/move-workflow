#include "workflow-trigger-filter.h"
#include "workflow-engine-service.h"
#include "workflow-model.h"
#include "workflow-persistence.h"
#include <obs.h>
#include <cstring>

namespace {
struct trigger_filter {
    obs_source_t *source;
    bool enabled;
    char workflow_id[WORKFLOW_MAX_NAME];
    char trigger_id[WORKFLOW_MAX_NAME];
};

static const char *name(void *) { return "Trigger Workflow"; }

static void enabled_signal(void *param, calldata_t *calldata)
{
    trigger_filter *data = static_cast<trigger_filter *>(param);
    if (!data)
        return;

    const bool enabled = calldata_bool(calldata, "enabled");
    if (enabled == data->enabled)
        return;

    data->enabled = enabled;
    if (!enabled || !data->workflow_id[0] || !data->trigger_id[0])
        return;

    workflow_engine_service_trigger(data->workflow_id, data->trigger_id);
}

static void *create(obs_data_t *settings, obs_source_t *source)
{
    trigger_filter *data = new trigger_filter{};
    data->source = source;
    data->enabled = obs_source_enabled(source);
    std::strncpy(data->workflow_id, obs_data_get_string(settings, "workflow"),
                 sizeof(data->workflow_id) - 1);
    std::strncpy(data->trigger_id, obs_data_get_string(settings, "trigger"),
                 sizeof(data->trigger_id) - 1);

    signal_handler_t *handler = obs_source_get_signal_handler(source);
    if (handler)
        signal_handler_connect(handler, "enable", enabled_signal, data);
    return data;
}

static void destroy(void *opaque)
{
    trigger_filter *data = static_cast<trigger_filter *>(opaque);
    if (!data)
        return;
    signal_handler_t *handler = obs_source_get_signal_handler(data->source);
    if (handler)
        signal_handler_disconnect(handler, "enable", enabled_signal, data);
    delete data;
}

static void update(void *opaque, obs_data_t *settings)
{
    trigger_filter *data = static_cast<trigger_filter *>(opaque);
    if (!data)
        return;
    std::strncpy(data->workflow_id, obs_data_get_string(settings, "workflow"),
                 sizeof(data->workflow_id) - 1);
    std::strncpy(data->trigger_id, obs_data_get_string(settings, "trigger"),
                 sizeof(data->trigger_id) - 1);
}

static void fill_triggers(obs_property_t *property, const workflow_t *workflow)
{
    obs_property_list_clear(property);
    if (!workflow) {
        obs_property_list_add_string(property, "Select a workflow first", "");
        return;
    }

    bool found = false;
    for (size_t i = 0; i < workflow->node_count; ++i) {
        const workflow_node_t &node = workflow->nodes[i];
        if (node.type != WORKFLOW_NODE_TRIGGER)
            continue;
        obs_property_list_add_string(property, node.name, node.id);
        found = true;
    }
    if (!found)
        obs_property_list_add_string(property, "No triggers in this workflow", "");
}

static bool workflow_modified(obs_properties_t *props, obs_property_t *, obs_data_t *settings)
{
    workflow_manager_t *manager = workflow_persistence_manager();
    workflow_t *workflow = manager
        ? workflow_manager_find(manager, obs_data_get_string(settings, "workflow"))
        : nullptr;
    fill_triggers(obs_properties_get(props, "trigger"), workflow);
    return true;
}

static obs_properties_t *properties(void *)
{
    obs_properties_t *props = obs_properties_create();
    obs_property_t *workflow = obs_properties_add_list(
        props, "workflow", "Workflow", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_t *trigger = obs_properties_add_list(
        props, "trigger", "Trigger", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(trigger, "Select a workflow first", "");
    obs_property_set_modified_callback(workflow, workflow_modified);

    workflow_manager_t *manager = workflow_persistence_manager();
    if (manager) {
        for (size_t i = 0; i < manager->workflow_count; ++i)
            obs_property_list_add_string(workflow, manager->workflows[i].name,
                                         manager->workflows[i].id);
    }
    return props;
}

static obs_source_info info = []() {
    obs_source_info value{};
    value.id = "move_workflow_trigger_filter";
    value.type = OBS_SOURCE_TYPE_FILTER;
    value.output_flags = OBS_SOURCE_VIDEO;
    value.get_name = name;
    value.create = create;
    value.destroy = destroy;
    value.update = update;
    value.get_properties = properties;
    return value;
}();
}

void workflow_trigger_filter_register(void)
{
    obs_register_source(&info);
}
