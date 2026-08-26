#include "workflow-trigger-filter.h"

#include "workflow-engine-service.h"
#include "workflow-model.h"
#include "workflow-persistence.h"

#include <obs.h>

#include <cstring>

namespace {
struct trigger_filter {
	obs_source_t *source;
	bool visible;
	char workflow_id[WORKFLOW_MAX_NAME];
	char trigger_id[WORKFLOW_MAX_NAME];
};

static const char *id(void *) { return "move_workflow_trigger_filter"; }
static const char *name(void *) { return "Trigger Workflow"; }

static void *create(obs_data_t *settings, obs_source_t *source)
{
	auto *data = new trigger_filter{};
	data->source = source;
	obs_data_get_string(settings, "workflow");
	return data;
}

static void destroy(void *opaque)
{
	delete static_cast<trigger_filter *>(opaque);
}

static void update(void *opaque, obs_data_t *settings)
{
	auto *data = static_cast<trigger_filter *>(opaque);
	if (!data)
		return;
	std::strncpy(data->workflow_id, obs_data_get_string(settings, "workflow"),
				 sizeof(data->workflow_id) - 1);
	std::strncpy(data->trigger_id, obs_data_get_string(settings, "trigger"),
				 sizeof(data->trigger_id) - 1);
}

static void show(void *opaque)
{
	auto *data = static_cast<trigger_filter *>(opaque);
	if (!data || data->visible)
		return;
	data->visible = true;
	if (!data->workflow_id[0] || !data->trigger_id[0])
		return;
	workflow_engine_service_trigger(data->workflow_id, data->trigger_id);
}

static void hide(void *opaque)
{
	auto *data = static_cast<trigger_filter *>(opaque);
	if (data)
		data->visible = false;
}

static obs_properties_t *properties(void *)
{
	auto *props = obs_properties_create();
	auto *workflow = obs_properties_add_list(props, "workflow", "Workflow",
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	auto *trigger = obs_properties_add_list(props, "trigger", "Trigger",
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	auto *manager = workflow_persistence_manager();
	if (manager) {
		for (size_t i = 0; i < manager->workflow_count; ++i) {
			const workflow_t &w = manager->workflows[i];
			obs_property_list_add_string(workflow, w.name, w.id);
			for (size_t n = 0; n < w.node_count; ++n) {
				const workflow_node_t &node = w.nodes[n];
				if (node.type == WORKFLOW_NODE_TRIGGER)
					obs_property_list_add_string(trigger, node.name, node.id);
			}
		}
	}
	return props;
}

static obs_source_info info = [] {
	obs_source_info value{};
	value.id = "move_workflow_trigger_filter";
	value.type = OBS_SOURCE_TYPE_FILTER;
	value.output_flags = OBS_SOURCE_VIDEO;
	value.get_name = name;
	value.create = create;
	value.destroy = destroy;
	value.update = update;
	value.get_properties = properties;
	value.show = show;
	value.hide = hide;
	return value;
}();
} // namespace

void workflow_trigger_filter_register(void)
{
	obs_register_source(&info);
}
