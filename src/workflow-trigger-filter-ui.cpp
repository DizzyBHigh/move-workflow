#include "workflow-trigger-filter-ui.h"
#include "workflow-model.h"
#include "workflow-persistence.h"
#include <obs.h>

namespace {
static bool workflow_modified(obs_properties_t *props, obs_property_t *, obs_data_t *settings)
{
    auto *manager = workflow_persistence_manager();
    auto *workflow = manager ? workflow_manager_find(manager, obs_data_get_string(settings, "workflow")) : nullptr;
    auto *trigger = obs_properties_get(props, "trigger");
    if (!trigger)
        return true;

    obs_property_list_clear(trigger);
    if (workflow) {
        for (size_t i = 0; i < workflow->node_count; ++i) {
            if (workflow->nodes[i].type == WORKFLOW_NODE_TRIGGER)
                obs_property_list_add_string(trigger, workflow->nodes[i].name, workflow->nodes[i].id);
        }
    }
    return true;
}
}

obs_properties_t *workflow_trigger_filter_properties(void *)
{
    auto *props = obs_properties_create();
    auto *workflow = obs_properties_add_list(props, "workflow", "Workflow", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    auto *trigger = obs_properties_add_list(props, "trigger", "Workflow Trigger", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

    obs_property_list_add_string(trigger, "Select a workflow first", "");
    obs_property_set_modified_callback(workflow, workflow_modified);

    if (auto *manager = workflow_persistence_manager()) {
        for (size_t i = 0; i < manager->workflow_count; ++i)
            obs_property_list_add_string(workflow, manager->workflows[i].name, manager->workflows[i].id);
    }

    return props;
}
