#include "workflow-filter-instance.h"

#include "workflow-debug.h"

#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <util/platform.h>

static obs_source_t *duplicate_filter(obs_source_t *original)
{
	return original ? obs_source_duplicate(original, obs_source_get_name(original), true) : nullptr;
}

static void clear_chaining(obs_source_t *filter)
{
	obs_data_t *settings = filter ? obs_source_get_settings(filter) : nullptr;
	if (!settings) return;
	obs_data_set_string(settings, "simultaneous_move", "");
	obs_data_set_string(settings, "next_move", "");
	obs_data_set_string(settings, "next_move_on", "move_end");
	obs_source_update(filter, settings);
	obs_data_release(settings);
}

static void apply_node_settings(obs_source_t *filter, const workflow_node_t *node,
								uint64_t *duration_ms)
{
	obs_data_t *settings = obs_source_get_settings(filter);
	if (!settings) return;
	*duration_ms = (uint64_t)obs_data_get_int(settings, "duration");
	if (node->duration.mode == WORKFLOW_OVERRIDE) {
		*duration_ms = node->duration.duration_ms;
		obs_data_set_bool(settings, "custom_duration", true);
		obs_data_set_int(settings, "duration", (long long)*duration_ms);
	}
	/* Let the native Move filter start from its normal enable-trigger path.
	 * The duplicated filter is initially disabled, so the subsequent enable
	 * produces the same native start event without reimplementing Move. */
	obs_data_set_int(settings, "start_trigger", 5);
	workflow_debug_log("Move dispatch: configuring native ENABLE trigger");
	obs_source_update(filter, settings);
	obs_data_release(settings);
}

workflow_filter_instance *workflow_filter_instance_create(
	obs_source_t *original, obs_source_t *parent, const workflow_node_t *node)
{
	if (!original || !parent || !node) return nullptr;
	workflow_filter_instance *result =
		(workflow_filter_instance *)calloc(1, sizeof(*result));
	if (!result) return nullptr;
	result->original = obs_source_get_ref(original);
	result->parent = obs_source_get_ref(parent);
	result->instance = duplicate_filter(original);
	if (!result->instance) {
		workflow_filter_instance_destroy(result);
		return nullptr;
	}
	clear_chaining(result->instance);
	obs_source_filter_add(parent, result->instance);
	workflow_debug_log("Filter instance: duplicated '%s' for node='%s'",
		obs_source_get_name(original), node->id);
	return result;
}

bool workflow_filter_instance_execute(workflow_filter_instance *instance)
{
	if (!instance || !instance->instance) return false;
	obs_source_set_enabled(instance->instance, true);
	workflow_debug_log("Filter instance: enabled temporary '%s'",
		obs_source_get_name(instance->instance));
	return true;
}

void workflow_filter_instance_destroy(workflow_filter_instance *instance)
{
	if (!instance) return;
	if (instance->parent && instance->instance)
		obs_source_filter_remove(instance->parent, instance->instance);
	if (instance->instance) obs_source_release(instance->instance);
	if (instance->parent) obs_source_release(instance->parent);
	if (instance->original) obs_source_release(instance->original);
	free(instance);
}

typedef struct destroy_context {
	workflow_filter_instance *instance;
	uint64_t delay_ms;
} destroy_context;

static void destroy_on_ui(void *data)
{
	destroy_context *ctx = (destroy_context *)data;
	if (!ctx) return;
	workflow_debug_log("Filter instance: destroying temporary filter");
	workflow_filter_instance_destroy(ctx->instance);
	free(ctx);
}

static void *destroy_thread(void *data)
{
	destroy_context *ctx = (destroy_context *)data;
	if (!ctx) return NULL;
	os_sleep_ms((uint32_t)ctx->delay_ms);
	obs_queue_task(OBS_TASK_UI, destroy_on_ui, ctx, false);
	return NULL;
}

static void schedule_destroy(workflow_filter_instance *instance, uint64_t duration_ms)
{
	destroy_context *ctx = (destroy_context *)calloc(1, sizeof(*ctx));
	if (!ctx) return;
	ctx->instance = instance;
	ctx->delay_ms = duration_ms ? duration_ms : 1;
	pthread_t thread;
	if (pthread_create(&thread, NULL, destroy_thread, ctx) != 0)
		free(ctx);
	else
		pthread_detach(thread);
}

bool workflow_filter_instance_execute_node(workflow_t *workflow, workflow_node_t *node)
{
	if (!workflow || !node || !workflow->enabled) return false;
	obs_source_t *parent = obs_get_source_by_name(node->action.scene_name);
	if (!parent) return false;
	obs_source_t *original = obs_source_get_filter_by_name(parent, node->action.filter_name);
	if (!original) {
		obs_source_release(parent);
		return false;
	}
	const char *expected = workflow_expected_filter_id(node->action.kind);
	const char *actual = obs_source_get_id(original);
	if (!expected || !actual || strcmp(expected, actual) != 0) {
		obs_source_release(original);
		obs_source_release(parent);
		return false;
	}
	workflow_filter_instance *instance =
		workflow_filter_instance_create(original, parent, node);
	obs_source_release(original);
	obs_source_release(parent);
	if (!instance) return false;
	uint64_t duration_ms = 0;
	apply_node_settings(instance->instance, node, &duration_ms);
	if (!workflow_filter_instance_execute(instance)) {
		workflow_filter_instance_destroy(instance);
		return false;
	}
	schedule_destroy(instance, duration_ms);
	return true;
}
