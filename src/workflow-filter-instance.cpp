#include "workflow-filter-instance.h"

#include "workflow-debug.h"
#include "workflow-engine-node.h"

#include <cstdlib>

static obs_source_t *duplicate_filter(obs_source_t *original)
{
	if (!original)
		return nullptr;

	return obs_source_duplicate(original, obs_source_get_name(original), true);
}

static void clear_chaining(obs_source_t *filter)
{
	if (!filter)
		return;

	obs_data_t *settings = obs_source_get_settings(filter);
	if (!settings)
		return;

	/* Workflow owns sequencing; native Move chaining must remain disabled. */
	obs_data_set_string(settings, "simultaneous_move", "");
	obs_data_set_string(settings, "next_move", "");
	obs_data_set_string(settings, "next_move_on", "move_end");
	obs_source_update(filter, settings);
	obs_data_release(settings);
}

workflow_filter_instance *workflow_filter_instance_create(
	obs_source_t *original, obs_source_t *parent, const workflow_node *node)
{
	if (!original || !parent || !node)
		return nullptr;

	workflow_filter_instance *instance =
		(workflow_filter_instance *)calloc(1, sizeof(*instance));
	if (!instance)
		return nullptr;

	instance->original = original;
	instance->parent = parent;
	obs_source_addref(original);
	obs_source_addref(parent);

	instance->instance = duplicate_filter(original);
	if (!instance->instance) {
		workflow_filter_instance_destroy(instance);
		return nullptr;
	}

	clear_chaining(instance->instance);
	obs_source_filter_add(parent, instance->instance);

	workflow_debug_log("Filter instance: duplicated '%s' for workflow node '%s'",
			obs_source_get_name(original), node->id);
	return instance;
}

bool workflow_filter_instance_execute(workflow_filter_instance *instance)
{
	if (!instance || !instance->instance)
		return false;

	obs_source_set_enabled(instance->instance, true);
	workflow_debug_log("Filter instance: executing temporary filter '%s'",
			obs_source_get_name(instance->instance));
	return true;
}

void workflow_filter_instance_destroy(workflow_filter_instance *instance)
{
	if (!instance)
		return;

	if (instance->parent && instance->instance)
		obs_source_filter_remove(instance->parent, instance->instance);

	if (instance->instance)
		obs_source_release(instance->instance);
	if (instance->parent)
		obs_source_release(instance->parent);
	if (instance->original)
		obs_source_release(instance->original);
	free(instance);
}
