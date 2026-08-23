#include <obs-module.h>
#include <obs.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/platform.h>
#include <util/threading.h>

#include "workflow-model.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEST_SCENE_NAME "obs-move-workflow test scene"
#define PHASE12_TEST_DURATION_MS 1000
#define PHASE13_END_ACTION_DELAY_MS 350

typedef struct duration_restore_context {
	obs_source_t *filter;
	bool custom_duration;
	long long duration;
} duration_restore_context_t;

typedef struct end_action_context {
	workflow_t *workflow;
	char node_id[WORKFLOW_MAX_NAME];
	uint64_t delay_ms;
} end_action_context_t;

static workflow_t workflow = {
	.id = "phase14-test-workflow",
	.name = "Test Move Source Workflow",
	.enabled = true,
	.entry_node_count = 6,
	.entry_node_ids = {"move-left", "move-center", "move-right", "move-bottom-left", "move-bottom-center", "move-bottom-right"},
	.node_count = 6,
	.nodes = {
		{
			.id = "move-left",
			.name = "Move Source - Top - Left",
			.action = {.scene_name = TEST_SCENE_NAME, .source_name = "", .filter_name = "Move Source - Top - Left", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
			.duration = {.mode = WORKFLOW_USE_EXISTING},
			.end_actions_mode = WORKFLOW_OVERRIDE,
			.start_trigger_mode = WORKFLOW_OVERRIDE,
			.start_trigger_value = "Enable",
			.stop_trigger_mode = WORKFLOW_USE_EXISTING,
			.simultaneous_actions_mode = WORKFLOW_OVERRIDE,
			.next_actions_mode = WORKFLOW_OVERRIDE,
			.next_move_on_mode = WORKFLOW_OVERRIDE,
			.end_node_count = 1,
			.end_node_ids = {"move-center"},
		},
		{
			.id = "move-center",
			.name = "Move Source - Top - Center",
			.action = {.scene_name = TEST_SCENE_NAME, .source_name = "", .filter_name = "Move Source - Top - Center", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
			.duration = {.mode = WORKFLOW_USE_EXISTING},
			.end_actions_mode = WORKFLOW_OVERRIDE,
			.start_trigger_mode = WORKFLOW_USE_EXISTING,
			.stop_trigger_mode = WORKFLOW_USE_EXISTING,
			.simultaneous_actions_mode = WORKFLOW_OVERRIDE,
			.next_actions_mode = WORKFLOW_OVERRIDE,
			.next_move_on_mode = WORKFLOW_OVERRIDE,
		},
		{
			.id = "move-right",
			.name = "Move Source - Top - Right",
			.action = {.scene_name = TEST_SCENE_NAME, .source_name = "", .filter_name = "Move Source - Top - Right", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
			.duration = {.mode = WORKFLOW_USE_EXISTING},
			.end_actions_mode = WORKFLOW_OVERRIDE,
			.start_trigger_mode = WORKFLOW_USE_EXISTING,
			.stop_trigger_mode = WORKFLOW_USE_EXISTING,
			.simultaneous_actions_mode = WORKFLOW_OVERRIDE,
			.next_actions_mode = WORKFLOW_OVERRIDE,
			.next_move_on_mode = WORKFLOW_OVERRIDE,
		},
		{
			.id = "move-bottom-left",
			.name = "Move Source - Bottom - Left",
			.action = {.scene_name = TEST_SCENE_NAME, .source_name = "", .filter_name = "Move Source - Bottom - Left", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
			.duration = {.mode = WORKFLOW_USE_EXISTING},
			.end_actions_mode = WORKFLOW_OVERRIDE,
			.start_trigger_mode = WORKFLOW_USE_EXISTING,
			.stop_trigger_mode = WORKFLOW_USE_EXISTING,
			.simultaneous_actions_mode = WORKFLOW_OVERRIDE,
			.next_actions_mode = WORKFLOW_OVERRIDE,
			.next_move_on_mode = WORKFLOW_OVERRIDE,
		},
		{
			.id = "move-bottom-center",
			.name = "Move Source - Bottom - Center",
			.action = {.scene_name = TEST_SCENE_NAME, .source_name = "", .filter_name = "Move Source - Bottom - Center", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
			.duration = {.mode = WORKFLOW_USE_EXISTING},
			.start_delay = {.mode = WORKFLOW_USE_EXISTING, .delay_ms = 0},
			.end_actions_mode = WORKFLOW_OVERRIDE,
			.start_trigger_mode = WORKFLOW_USE_EXISTING,
			.stop_trigger_mode = WORKFLOW_USE_EXISTING,
			.simultaneous_actions_mode = WORKFLOW_OVERRIDE,
			.next_actions_mode = WORKFLOW_OVERRIDE,
			.next_move_on_mode = WORKFLOW_USE_EXISTING,
		},
		{
			.id = "move-bottom-right",
			.name = "Move Source - Bottom - Right",
			.action = {.scene_name = TEST_SCENE_NAME, .source_name = "", .filter_name = "Move Source - Bottom - Right", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
			.duration = {.mode = WORKFLOW_USE_EXISTING},
			.start_delay = {.mode = WORKFLOW_USE_EXISTING, .delay_ms = 0},
			.end_actions_mode = WORKFLOW_OVERRIDE,
			.start_trigger_mode = WORKFLOW_USE_EXISTING,
			.stop_trigger_mode = WORKFLOW_USE_EXISTING,
			.simultaneous_actions_mode = WORKFLOW_OVERRIDE,
			.next_actions_mode = WORKFLOW_OVERRIDE,
			.next_move_on_mode = WORKFLOW_USE_EXISTING,
		},
	},
};

static obs_source_t *find_move_filter(const workflow_action_ref_t *action)
{
	const char *expected_id = workflow_expected_filter_id(action->kind);
	if (!expected_id || !strlen(expected_id) || strcmp(action->filter_id, expected_id) != 0)
		return NULL;

	obs_source_t *scene_source = obs_get_source_by_name(action->scene_name);
	if (!scene_source)
		return NULL;

	obs_source_t *filter = obs_source_get_filter_by_name(scene_source, action->filter_name);
	obs_source_release(scene_source);
	if (!filter)
		return NULL;

	if (strcmp(obs_source_get_id(filter), action->filter_id) != 0) {
		obs_source_release(filter);
		return NULL;
	}
	return filter;
}

static void inspect_existing_move_settings(obs_source_t *filter, const workflow_node_t *node)
{
	obs_data_t *settings = obs_source_get_settings(filter);
	if (!settings)
		return;

	const bool custom_duration = obs_data_get_bool(settings, "custom_duration");
	const long long duration = obs_data_get_int(settings, "duration");
	const long long start_trigger = obs_data_get_int(settings, "start_trigger");
	const long long stop_trigger = obs_data_get_int(settings, "stop_trigger");
	const long long next_move_on = obs_data_get_int(settings, "next_move_on");
	const char *source = obs_data_get_string(settings, "source");
	const char *simultaneous = obs_data_get_string(settings, "simultaneous_move");
	const char *next = obs_data_get_string(settings, "next_move");

	blog(LOG_INFO,
	     "[Move Workflow Phase 14] Existing settings for node \"%s\": source=\"%s\" custom_duration=%s duration=%lldms start_trigger=%lld stop_trigger=%lld simultaneous=\"%s\" next=\"%s\" next_move_on=%lld",
	     node->name, source ? source : "", custom_duration ? "true" : "false", duration,
	     start_trigger, stop_trigger, simultaneous ? simultaneous : "", next ? next : "", next_move_on);

	obs_data_release(settings);
}

static void restore_duration(void *data)
{
	duration_restore_context_t *ctx = data;
	if (!ctx)
		return;

	obs_data_t *settings = obs_source_get_settings(ctx->filter);
	if (settings) {
		obs_data_set_bool(settings, "custom_duration", ctx->custom_duration);
		obs_data_set_int(settings, "duration", ctx->duration);
		obs_source_update(ctx->filter, settings);
		obs_data_release(settings);
	}

	blog(LOG_INFO, "[Move Workflow Phase 14] Duration override restored: custom_duration=%s duration=%lldms",
	     ctx->custom_duration ? "true" : "false", ctx->duration);
	obs_source_release(ctx->filter);
	free(ctx);
}

static void *duration_restore_thread(void *data)
{
	duration_restore_context_t *ctx = data;
	os_set_thread_name("move-workflow-duration-restore");
	os_sleep_ms(PHASE12_TEST_DURATION_MS + 150);
	obs_queue_task(OBS_TASK_UI, restore_duration, ctx, false);
	return NULL;
}

static bool apply_duration_override(obs_source_t *filter, uint64_t duration_ms)
{
	obs_data_t *settings = obs_source_get_settings(filter);
	if (!settings)
		return false;

	duration_restore_context_t *ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		obs_data_release(settings);
		return false;
	}

	ctx->filter = obs_source_get_ref(filter);
	ctx->custom_duration = obs_data_get_bool(settings, "custom_duration");
	ctx->duration = obs_data_get_int(settings, "duration");
	obs_data_set_bool(settings, "custom_duration", true);
	obs_data_set_int(settings, "duration", (long long)duration_ms);
	obs_source_update(filter, settings);
	obs_data_release(settings);

	blog(LOG_INFO, "[Move Workflow Phase 14] Applying duration override: original=%lldms override=%llums",
	     ctx->duration, (unsigned long long)duration_ms);

	pthread_t thread;
	if (pthread_create(&thread, NULL, duration_restore_thread, ctx) != 0) {
		restore_duration(ctx);
		return false;
	}
	pthread_detach(thread);
	return true;
}

static workflow_node_t *find_node(workflow_t *wf, const char *node_id)
{
	for (size_t i = 0; i < wf->node_count; ++i) {
		if (strcmp(wf->nodes[i].id, node_id) == 0)
			return &wf->nodes[i];
	}
	return NULL;
}

static void execute_node(workflow_node_t *node);

static void execute_delayed_end_action(void *data)
{
	end_action_context_t *ctx = data;
	if (!ctx)
		return;

	workflow_node_t *end_node = find_node(ctx->workflow, ctx->node_id);
	if (end_node) {
		blog(LOG_INFO,
		     "[Move Workflow Phase 14] Director end action -> \"%s\" (start delay=%llums)",
		     end_node->name, (unsigned long long)ctx->delay_ms);
		execute_node(end_node);
	} else {
		blog(LOG_WARNING, "[Move Workflow Phase 14] End action node not found: \"%s\"", ctx->node_id);
	}

	free(ctx);
}

static void *end_action_thread(void *data)
{
	end_action_context_t *ctx = data;
	os_set_thread_name("move-workflow-end-action");
	os_sleep_ms(PHASE13_END_ACTION_DELAY_MS + (uint32_t)ctx->delay_ms);
	obs_queue_task(OBS_TASK_UI, execute_delayed_end_action, ctx, false);
	return NULL;
}

static void schedule_end_actions(workflow_t *wf, const workflow_node_t *node)
{
	if (node->end_actions_mode != WORKFLOW_OVERRIDE || node->end_node_count == 0)
		return;

	blog(LOG_INFO, "[Move Workflow Phase 14] Scheduling %zu director end action(s) for \"%s\"",
	     node->end_node_count, node->name);

	for (size_t i = 0; i < node->end_node_count; ++i) {
		end_action_context_t *ctx = calloc(1, sizeof(*ctx));
		if (!ctx)
			continue;

		ctx->workflow = wf;
		snprintf(ctx->node_id, WORKFLOW_MAX_NAME, "%s", node->end_node_ids[i]);

		workflow_node_t *end_node = find_node(wf, ctx->node_id);
		if (end_node && end_node->start_delay.mode == WORKFLOW_OVERRIDE)
			ctx->delay_ms = end_node->start_delay.delay_ms;

		blog(LOG_INFO,
		     "[Move Workflow Phase 14] End action %zu/%zu -> \"%s\" start delay=%llums",
		     i + 1, node->end_node_count, ctx->node_id, (unsigned long long)ctx->delay_ms);

		pthread_t thread;
		if (pthread_create(&thread, NULL, end_action_thread, ctx) != 0) {
			free(ctx);
			continue;
		}
		pthread_detach(thread);
	}
}

static void apply_start_trigger_override(obs_source_t *filter, const workflow_node_t *node)
{
	if (node->start_trigger_mode == WORKFLOW_OVERRIDE && strcmp(node->start_trigger_value, "Enable") == 0) {
		blog(LOG_INFO,
		     "[Move Workflow Phase 15] Applying director Start Trigger override: Enable -> \"%s\"",
		     node->name);
		obs_source_set_enabled(filter, false);
		obs_source_set_enabled(filter, true);
		return;
	}

	/* The current phase only defines the Enable override. Other trigger modes
	 * remain outside this test and will be added as their own director phases. */
	obs_source_set_enabled(filter, false);
	obs_source_set_enabled(filter, true);
}

static void execute_node(workflow_node_t *node)
{
	obs_source_t *filter = find_move_filter(&node->action);
	if (!filter) {
		blog(LOG_WARNING,
		     "[Move Workflow Phase 14] Target not found or type mismatch: node=\"%s\" scene=\"%s\" filter=\"%s\" id=\"%s\" kind=\"%s\"",
		     node->name, node->action.scene_name, node->action.filter_name, node->action.filter_id,
		     workflow_move_kind_name(node->action.kind));
		return;
	}

	blog(LOG_INFO,
	     "[Move Workflow Phase 14] Executing node: \"%s\" | action=%s | duration=%s | start-delay=%s | end=%s | start=%s | stop=%s | simultaneous=%s | next=%s | next-on=%s",
	     node->name, workflow_move_kind_name(node->action.kind), workflow_value_mode_name(node->duration.mode),
	     workflow_value_mode_name(node->start_delay.mode), workflow_value_mode_name(node->end_actions_mode),
	     workflow_value_mode_name(node->start_trigger_mode), workflow_value_mode_name(node->stop_trigger_mode),
	     workflow_value_mode_name(node->simultaneous_actions_mode), workflow_value_mode_name(node->next_actions_mode),
	     workflow_value_mode_name(node->next_move_on_mode));

	inspect_existing_move_settings(filter, node);

	if (node->duration.mode == WORKFLOW_OVERRIDE && !apply_duration_override(filter, node->duration.duration_ms)) {
		obs_source_release(filter);
		return;
	}

	/* Director-owned chaining: the selected filter's own next/end workflow is not inherited. */
	apply_start_trigger_override(filter, node);
	obs_source_release(filter);

	/* End Actions start after the parent action completes, with each child owning its own Start Delay. */
	schedule_end_actions(&workflow, node);
}

static void execute_node_by_id(workflow_t *wf, const char *node_id)
{
	if (!wf->enabled)
		return;

	workflow_node_t *node = find_node(wf, node_id);
	if (node)
		execute_node(node);
	else
		blog(LOG_WARNING, "[Move Workflow Phase 14] Test node not found: \"%s\"", node_id);
}

static void hotkey_left_callback(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	if (pressed)
		execute_node_by_id((workflow_t *)data, "move-left");
}

static void hotkey_center_callback(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	if (pressed)
		execute_node_by_id((workflow_t *)data, "move-center");
}

static void hotkey_right_callback(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	if (pressed)
		execute_node_by_id((workflow_t *)data, "move-right");
}

static void menu_left_cb(void *data) { execute_node_by_id((workflow_t *)data, "move-left"); }
static void menu_center_cb(void *data) { execute_node_by_id((workflow_t *)data, "move-center"); }
static void menu_right_cb(void *data) { execute_node_by_id((workflow_t *)data, "move-right"); }

static void menu_duration_cb(void *data)
{
	workflow_t *wf = data;
	workflow_node_t *node = find_node(wf, "move-left");
	if (!node)
		return;

	node->duration.mode = WORKFLOW_OVERRIDE;
	node->duration.duration_ms = PHASE12_TEST_DURATION_MS;
	execute_node(node);
	node->duration.mode = WORKFLOW_USE_EXISTING;
}

static void menu_end_action_cb(void *data)
{
	workflow_t *wf = data;
	workflow_node_t *node = find_node(wf, "move-left");
	if (!node)
		return;

	node->end_actions_mode = WORKFLOW_OVERRIDE;
	node->end_node_count = 1;
	snprintf(node->end_node_ids[0], WORKFLOW_MAX_NAME, "%s", "move-center");
	blog(LOG_INFO, "[Move Workflow Phase 14] TEST: Top Left has director end action -> Top Center");
	execute_node(node);
}

static void menu_multiple_end_actions_cb(void *data)
{
	workflow_t *wf = data;
	workflow_node_t *node = find_node(wf, "move-left");
	if (!node)
		return;

	workflow_node_t *bottom_right = find_node(wf, "move-bottom-right");
	if (bottom_right) {
		bottom_right->start_delay.mode = WORKFLOW_USE_EXISTING;
		bottom_right->start_delay.delay_ms = 0;
	}

	node->end_actions_mode = WORKFLOW_OVERRIDE;
	node->end_node_count = 2;
	snprintf(node->end_node_ids[0], WORKFLOW_MAX_NAME, "%s", "move-bottom-center");
	snprintf(node->end_node_ids[1], WORKFLOW_MAX_NAME, "%s", "move-bottom-right");

	blog(LOG_INFO,
	     "[Move Workflow Phase 14] TEST: Top Left has 2 director end actions -> Bottom Center + Bottom Right (parallel)");
	execute_node(node);
}

static void menu_multiple_end_actions_delay_cb(void *data)
{
	workflow_t *wf = data;
	workflow_node_t *node = find_node(wf, "move-left");
	workflow_node_t *bottom_right = find_node(wf, "move-bottom-right");
	if (!node || !bottom_right)
		return;

	bottom_right->start_delay.mode = WORKFLOW_OVERRIDE;
	bottom_right->start_delay.delay_ms = 1000;

	node->end_actions_mode = WORKFLOW_OVERRIDE;
	node->end_node_count = 2;
	snprintf(node->end_node_ids[0], WORKFLOW_MAX_NAME, "%s", "move-bottom-center");
	snprintf(node->end_node_ids[1], WORKFLOW_MAX_NAME, "%s", "move-bottom-right");

	blog(LOG_INFO,
	     "[Move Workflow Phase 14] TEST: Bottom Center starts immediately; Bottom Right has director Start Delay = 1000ms");
	execute_node(node);
}

bool obs_module_load(void)
{
	static obs_hotkey_id left_hotkey_id, center_hotkey_id, right_hotkey_id;

	blog(LOG_INFO, "[Move Workflow Phase 14] Loaded");

	left_hotkey_id = obs_hotkey_register_frontend("obs_move_workflow.test_left", "Move Workflow: Test Left", hotkey_left_callback, &workflow);
	center_hotkey_id = obs_hotkey_register_frontend("obs_move_workflow.test_center", "Move Workflow: Test Center", hotkey_center_callback, &workflow);
	right_hotkey_id = obs_hotkey_register_frontend("obs_move_workflow.test_right", "Move Workflow: Test Right", hotkey_right_callback, &workflow);
	UNUSED_PARAMETER(left_hotkey_id);
	UNUSED_PARAMETER(center_hotkey_id);
	UNUSED_PARAMETER(right_hotkey_id);

	obs_frontend_add_tools_menu_item("Move Workflow: Test Left", menu_left_cb, &workflow);
	obs_frontend_add_tools_menu_item("Move Workflow: Test Center", menu_center_cb, &workflow);
	obs_frontend_add_tools_menu_item("Move Workflow: Test Right", menu_right_cb, &workflow);
	obs_frontend_add_tools_menu_item("Move Workflow: Test Duration Override (Left)", menu_duration_cb, &workflow);
	obs_frontend_add_tools_menu_item("Move Workflow: Test End Action Left -> Center", menu_end_action_cb, &workflow);
	obs_frontend_add_tools_menu_item("Move Workflow: Test Multiple End Actions (Top Left -> Bottom Center + Bottom Right)", menu_multiple_end_actions_cb, &workflow);
	obs_frontend_add_tools_menu_item("Move Workflow: Test End Actions with Start Delay (Bottom Right +1000ms)", menu_multiple_end_actions_delay_cb, &workflow);

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[Move Workflow Phase 14] Unloaded");
}
