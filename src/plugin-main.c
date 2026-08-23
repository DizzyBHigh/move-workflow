#include <obs-module.h>
#include <obs.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <string.h>

#include "workflow-model.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEST_SCENE_NAME "obs-move-workflow test scene"
#define MAX_ACTIONS 8

typedef struct workflow {
	char name[WORKFLOW_MAX_NAME];
	bool enabled;
	workflow_move_action_t actions[MAX_ACTIONS];
	size_t action_count;
} workflow_t;

/* Phase 8 deliberately keeps the test target the same as the proven Phase 6/7
 * test. The important change here is the workflow representation: a Move
 * action has an explicit kind plus Source, Swap and Value fields, each of
 * which can inherit the existing Move filter setting or request an override.
 */
static workflow_t workflow = {
	.name = "Test Move Source Workflow",
	.enabled = true,
	.actions = {
		{
			.scene_name = TEST_SCENE_NAME,
			.filter_name = "Move Source - Left",
			.filter_id = "move_source_filter",
			.kind = WORKFLOW_MOVE_SOURCE,
			.source_mode = WORKFLOW_USE_EXISTING,
			.swap_mode = WORKFLOW_USE_EXISTING,
			.value_mode = WORKFLOW_USE_EXISTING,
		},
	},
	.action_count = 1,
};

static obs_source_t *find_move_filter(const workflow_move_action_t *action)
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

static void execute_move_action(const workflow_move_action_t *action, size_t index)
{
	obs_source_t *filter = find_move_filter(action);
	if (!filter) {
		blog(LOG_WARNING,
		     "[Move Workflow Phase 8] Action %zu target not found or type mismatch: scene=\"%s\" filter=\"%s\" id=\"%s\" kind=\"%s\"",
		     index + 1, action->scene_name, action->filter_name, action->filter_id,
		     workflow_move_kind_name(action->kind));
		return;
	}

	blog(LOG_INFO,
	     "[Move Workflow Phase 8] Action %zu: %s | source=%s%s | swap=%s%s | value=%s%s",
	     index + 1,
	     workflow_move_kind_name(action->kind),
	     workflow_value_mode_name(action->source_mode),
	     action->source_mode == WORKFLOW_OVERRIDE ? action->source_value : "",
	     workflow_value_mode_name(action->swap_mode),
	     action->swap_mode == WORKFLOW_OVERRIDE ? action->swap_value : "",
	     workflow_value_mode_name(action->value_mode),
	     action->value_mode == WORKFLOW_OVERRIDE ? action->value_value : "");

	/* Phase 8 intentionally executes the existing Move configuration unchanged.
	 * Override application is a later phase because each Move filter family has
	 * different OBS settings and we want to apply and restore those settings
	 * without disturbing the user's saved Move configuration.
	 */
	obs_source_set_enabled(filter, false);
	obs_source_set_enabled(filter, true);
	obs_source_release(filter);
}

static void execute_workflow(workflow_t *wf)
{
	if (!wf->enabled)
		return;

	blog(LOG_INFO, "[Move Workflow Phase 8] Executing workflow: \"%s\" (%zu action(s))",
	     wf->name, wf->action_count);

	for (size_t i = 0; i < wf->action_count; ++i)
		execute_move_action(&wf->actions[i], i);
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

	blog(LOG_INFO, "[Move Workflow Phase 8] Loaded");
	blog(LOG_INFO, "[Move Workflow Phase 8] Workflow \"%s\" has %zu action(s)",
	     workflow.name, workflow.action_count);

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
	blog(LOG_INFO, "[Move Workflow Phase 8] Unloaded");
}
