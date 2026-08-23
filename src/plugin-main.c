#include <obs-module.h>
#include <obs.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <string.h>

#include "workflow-model.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEST_SCENE_NAME "obs-move-workflow test scene"

/*
 * Phase 9 uses the director node model from workflow-model.h directly.
 * The selected action is an existing Move-family filter; the node contains
 * only the director settings that may later be kept or overridden.
 *
 * For this phase every director setting remains WORKFLOW_USE_EXISTING.
 * Phase 10 adds the first runtime step toward overrides: read the actual
 * settings from the selected prebuilt Move filter so the director can resolve
 * "use existing" values without changing the user's configuration.
 */
static workflow_t workflow = {
	.id = "phase10-test-workflow",
	.name = "Test Move Source Workflow",
	.enabled = true,
	.entry_node_count = 1,
	.entry_node_ids = {"move-left"},
	.node_count = 1,
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
			.duration = {
				.mode = WORKFLOW_USE_EXISTING,
				.duration_ms = 0,
			},
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

/*
 * Read the settings that already belong to the user's Move filter.
 *
 * Nothing is written back to the filter here. This is deliberately the safe
 * half of the eventual override mechanism: first resolve the existing values,
 * then a later phase can merge director overrides into a temporary settings
 * object and restore the original configuration after execution.
 */
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
	     "[Move Workflow Phase 10] Existing settings for node \"%s\": source=\"%s\" custom_duration=%s duration=%lldms start_trigger=%lld stop_trigger=%lld simultaneous=\"%s\" next=\"%s\" next_move_on=%lld",
	     node->name,
	     source ? source : "",
	     custom_duration ? "true" : "false",
	     duration,
	     start_trigger,
	     stop_trigger,
	     simultaneous ? simultaneous : "",
	     next ? next : "",
	     next_move_on);

	/* Move Action has one built-in end-action definition. Our director model
	 * deliberately represents end actions as multiple node references, so this
	 * is only an inspection of the prebuilt action and not our final structure.
	 */
	if (strcmp(node->action.filter_id, "move_action_filter") == 0) {
		const long long end_action = obs_data_get_int(settings, "end_action");
		const char *end_source = obs_data_get_string(settings, "end_source");
		const char *end_filter = obs_data_get_string(settings, "end_filter");
		const long long end_enable = obs_data_get_int(settings, "end_enable");

		blog(LOG_INFO,
		     "[Move Workflow Phase 10] Existing Move Action end action: action=%lld source=\"%s\" filter=\"%s\" enable=%lld",
		     end_action,
		     end_source ? end_source : "",
		     end_filter ? end_filter : "",
		     end_enable);
	}

	obs_data_release(settings);
}

static void execute_node(const workflow_node_t *node, size_t index)
{
	obs_source_t *filter = find_move_filter(&node->action);
	if (!filter) {
		blog(LOG_WARNING,
		     "[Move Workflow Phase 10] Node %zu target not found or type mismatch: node=\"%s\" scene=\"%s\" filter=\"%s\" id=\"%s\" kind=\"%s\"",
		     index + 1,
		     node->name,
		     node->action.scene_name,
		     node->action.filter_name,
		     node->action.filter_id,
		     workflow_move_kind_name(node->action.kind));
		return;
	}

	blog(LOG_INFO,
	     "[Move Workflow Phase 10] Node %zu: \"%s\" | action=%s | duration=%s | end=%s | start=%s | stop=%s | simultaneous=%s | next=%s | next-on=%s",
	     index + 1,
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

	/* No overrides are applied yet. The selected Move filter is still executed
	 * exactly as the OBS user configured it. */
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

static void execute_workflow(workflow_t *wf)
{
	if (!wf->enabled)
		return;

	blog(LOG_INFO, "[Move Workflow Phase 10] Executing workflow: \"%s\" (%zu node(s))",
	     wf->name, wf->node_count);

	for (size_t i = 0; i < wf->entry_node_count; ++i) {
		workflow_node_t *node = find_node(wf, wf->entry_node_ids[i]);
		if (node)
			execute_node(node, i);
		else
			blog(LOG_WARNING, "[Move Workflow Phase 10] Entry node not found: \"%s\"",
			     wf->entry_node_ids[i]);
	}
}

static void workflow_hotkey_callback(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (pressed)
		execute_workflow(&workflow);
}

static void menu_cb(void *data)
{
	UNUSED_PARAMETER(data);
	execute_workflow(&workflow);
}

bool obs_module_load(void)
{
	static obs_hotkey_id hotkey_id;

	blog(LOG_INFO, "[Move Workflow Phase 10] Loaded");
	blog(LOG_INFO, "[Move Workflow Phase 10] Workflow \"%s\" has %zu node(s)",
	     workflow.name, workflow.node_count);

	hotkey_id = obs_hotkey_register_frontend(
		"obs_move_workflow.test_workflow",
		"Move Workflow: Run Test Move Source Workflow",
		workflow_hotkey_callback,
		&workflow);
	UNUSED_PARAMETER(hotkey_id);

	obs_frontend_add_tools_menu_item(
		"Move Workflow: Run Test Move Source Workflow", menu_cb, NULL);

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[Move Workflow Phase 10] Unloaded");
}
