#include <obs-module.h>
#include <obs.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <string.h>
#include <stdlib.h>

#include "workflow-model.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEST_SCENE_NAME "obs-move-workflow test scene"
#define PHASE12_TEST_DURATION_MS 1000

typedef struct duration_restore_context {
	obs_source_t *filter;
	bool custom_duration;
	long long duration;
} duration_restore_context_t;

static workflow_t workflow = {
	.id = "phase12-test-workflow",
	.name = "Test Move Source Workflow",
	.enabled = true,
	.entry_node_count = 3,
	.entry_node_ids = {"move-left", "move-center", "move-right"},
	.node_count = 3,
	.nodes = {
		{
			.id = "move-left",
			.name = "Move Source - Left",
			.action = {
				.scene_name = TEST_SCENE_NAME,
				.source_name = "",
				.filter_name = "Move Source - Left",
				.filter_id = "move_source_filter",
				.kind = WORKFLOW_MOVE_SOURCE,
			},
			.duration = {.mode = WORKFLOW_USE_EXISTING},
			.end_actions_mode = WORKFLOW_USE_EXISTING,
			.start_trigger_mode = WORKFLOW_USE_EXISTING,
			.stop_trigger_mode = WORKFLOW_USE_EXISTING,
			.simultaneous_actions_mode = WORKFLOW_USE_EXISTING,
			.next_actions_mode = WORKFLOW_USE_EXISTING,
			.next_move_on_mode = WORKFLOW_USE_EXISTING,
		},
		{
			.id = "move-center",
			.name = "Move Source - Center",
			.action = {
				.scene_name = TEST_SCENE_NAME,
				.source_name = "",
				.filter_name = "Move Source - Center",
				.filter_id = "move_source_filter",
				.kind = WORKFLOW_MOVE_SOURCE,
			},
			.duration = {.mode = WORKFLOW_USE_EXISTING},
			.end_actions_mode = WORKFLOW_USE_EXISTING,
			.start_trigger_mode = WORKFLOW_USE_EXISTING,
			.stop_trigger_mode = WORKFLOW_USE_EXISTING,
			.simultaneous_actions_mode = WORKFLOW_USE_EXISTING,
			.next_actions_mode = WORKFLOW_USE_EXISTING,
			.next_move_on_mode = WORKFLOW_USE_EXISTING,
		},
		{
			.id = "move-right",
			.name = "Move Source - Right",
			.action = {
				.scene_name = TEST_SCENE_NAME,
				source_name = "",
				.filter_name = "Move Source - Right",
				.filter_id = "move_source_filter",
				.kind = WORKFLOW_MOVE_SOURCE,
			},
			.duration = {.mode = WORKFLOW_USE_EXISTING},
			.end_actions_mode = WORKFLOW_USE_EXISTING,
			.start_trigger_mode = WORKFLOW_USE_EXISTING,
			.stop_trigger_mode = WORKFLOW_USE_EXISTING,
			.simultaneous_actions_mode = WORKFLOW_USE_EXISTING,
			.next_actions_mode = WORKFLOW_USE_EXISTING,
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
	     "[Move Workflow Phase 12] Existing settings for node \"%s\": source=\"%s\" custom_duration=%s duration=%lldms start_trigger=%lld stop_trigger=%lld simultaneous=\"%s\" next=\"%s\" next_move_on=%lld",
	     node->name,
	     source ? source : "",
	     custom_duration ? "true" : "false",
	     duration,
	     start_trigger,
	     stop_trigger,
	     simultaneous ? simultaneous : "",
	     next ? next : "",
	     next_move_on);

	if (node->action.kind == WORKFLOW_MOVE_ACTION) {
		const long long end_action = obs_data_get_int(settings, "end_action");
		const char *end_source = obs_data_get_string(settings, "end_source");
		const char *end_filter = obs_data_get_string(settings, "end_filter");
		const long long end_enable = obs_data_get_int(settings, "end_enable");

		blog(LOG_INFO,
		     "[Move Workflow Phase 12] Existing Move Action end action: action=%lld source=\"%s\" filter=\"%s\" enable=%lld",
		     end_action,
		     end_source ? end_source : "",
		     end_filter ? end_filter : "",
		     end_enable);
	}

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

	blog(LOG_INFO,
	     "[Move Workflow Phase 12] Duration override restored: custom_duration=%s duration=%lldms",
	     ctx->custom_duration ? "true" : "false",
	     ctx->duration);

	obs_source_release(ctx->filter);
	free(ctx);
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

	ctx->filter = filter;
	obs_source_addref(filter);
	ctx->custom_duration = obs_data_get_bool(settings, "custom_duration");
	ctx->duration = obs_data_get_int(settings, "duration");

	obs_data_set_bool(settings, "custom_duration", true);
	obs_data_set_int(settings, "duration", (long long)duration_ms);
	obs_source_update(filter, settings);
	obs_data_release(settings);

	blog(LOG_INFO,
	     "[Move Workflow Phase 12] Applying duration override: original=%lldms override=%llums",
	     ctx->duration,
	     (unsigned long long)duration_ms);

	obs_timer_add(restore_duration, ctx, duration_ms + 100);
	return true;
}

static void execute_node(const workflow_node_t *node)
{
	obs_source_t *filter = find_move_filter(&node->action);
	if (!filter) {
		blog(LOG_WARNING,
		     "[Move Workflow Phase 12] Target not found or type mismatch: node=\"%s\" scene=\"%s\" filter=\"%s\" id=\"%s\" kind=\"%s\"",
		     node->name,
		     node->action.scene_name,
		     node->action.filter_name,
		     node->action.filter_id,
		     workflow_move_kind_name(node->action.kind));
		return;
	}

	blog(LOG_INFO,
	     "[Move Workflow Phase 12] Executing node: \"%s\" | action=%s | duration=%s | end=%s | start=%s | stop=%s | simultaneous=%s | next=%s | next-on=%s",
	     node->name,
	     workflow_move_kind_name(node->action.kind),
	     workflow_value_mode_name(node->duration.mode),
	     workflow_value_mode_name(node->end_actions_mode),
	     workflow_value_mode_name(node->start_trigger_mode),
	     workflow_value_mode_name(node->stop_trigger_mode),
	     workflow_value_mode_name(node->simultaneous_actions_mode),
	     workflow_value_mode_name(node->next_actions_mode),
	     workflow_value_mode_name(node->next_move_on_mode));

	inspect_existing_move_settings(filter, node);

	if (node->duration.mode == WORKFLOW_OVERRIDE) {
		if (!apply_duration_override(filter, node->duration.duration_ms)) {
			blog(LOG_WARNING, "[Move Workflow Phase 12] Failed to apply duration override");
			obs_source_release(filter);
			return;
		}
	}

	/* The existing Move filter remains responsible for performing the move. */
	obs_source_set_enabled(filter, false);
	obs_source_set_enabled(filter, true);
	obs_source_release(filter);
}

static workflow_node_t *find_node(workflow_t *wf, const char *node_id)
{
	for (size_t i = 0; i < wf->node_count; ++i) {
		if (strcmp(wf->nodes[i].id, node_id) == 0)
			return &wf->nodes[i];
	}
	return NULL;
}

static void execute_node_by_id(workflow_t *wf, const char *node_id)
{
	if (!wf->enabled)
		return;

	workflow_node_t *node = find_node(wf, node_id);
	if (node)
		execute_node(node);
	else
		blog(LOG_WARNING, "[Move Workflow Phase 12] Test node not found: \"%s\"", node_id);
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

static void menu_left_cb(void *data)
{
	execute_node_by_id((workflow_t *)data, "move-left");
}

static void menu_center_cb(void *data)
{
	execute_node_by_id((workflow_t *)data, "move-center");
}

static void menu_right_cb(void *data)
{
	execute_node_by_id((workflow_t *)data, "move-right");
}

static void menu_duration_cb(void *data)
{
	workflow_t *wf = data;
	workflow_node_t *node = find_node(wf, "move-left");
	if (!node)
		return;

	node->duration.mode = WORKFLOW_OVERRIDE;
	node->duration.duration_ms = PHASE12_TEST_DURATION_MS;
	execute_node(node);
}

bool obs_module_load(void)
{
	static obs_hotkey_id left_hotkey_id;
	static obs_hotkey_id center_hotkey_id;
	static obs_hotkey_id right_hotkey_id;

	blog(LOG_INFO, "[Move Workflow Phase 12] Loaded");
	blog(LOG_INFO, "[Move Workflow Phase 12] Workflow \"%s\" has %zu node(s)",
	     workflow.name, workflow.node_count);

	left_hotkey_id = obs_hotkey_register_frontend(
		"obs_move_workflow.test_left",
		"Move Workflow: Test Left",
		hotkey_left_callback,
		&workflow);
	center_hotkey_id = obs_hotkey_register_frontend(
		"obs_move_workflow.test_center",
		"Move Workflow: Test Center",
		 hotkey_center_callback,
		&workflow);
	right_hotkey_id = obs_hotkey_register_frontend(
		"obs_move_workflow.test_right",
		"Move Workflow: Test Right",
		hotkey_right_callback,
		&workflow);
	UNUSED_PARAMETER(left_hotkey_id);
	UNUSED_PARAMETER(center_hotkey_id);
	UNUSED_PARAMETER(right_hotkey_id);

	obs_frontend_add_tools_menu_item("Move Workflow: Test Left", menu_left_cb, &workflow);
	obs_frontend_add_tools_menu_item("Move Workflow: Test Center", menu_center_cb, &workflow);
	obs_frontend_add_tools_menu_item("Move Workflow: Test Right", menu_right_cb, &workflow);
	obs_frontend_add_tools_menu_item("Move Workflow: Test Duration Override (Left)", menu_duration_cb, &workflow);

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[Move Workflow Phase 12] Unloaded");
}
