#include <obs-module.h>
#include <obs.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <string.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEST_SCENE_NAME "obs-move-workflow test scene"
#define MOVE_SOURCE_FILTER_ID "move_source_filter"
#define MAX_ACTIONS 8

typedef struct workflow_action {
	char scene_name[256];
	char filter_name[256];
	char filter_id[128];
} workflow_action_t;

typedef struct workflow {
	char name[256];
	bool enabled;
	workflow_action_t actions[MAX_ACTIONS];
	size_t action_count;
} workflow_t;

static workflow_t workflow = {
	.name = "Test Hotkey Workflow",
	.enabled = true,
	.actions = {
		{
			.scene_name = TEST_SCENE_NAME,
			.filter_name = "Move Source - Left",
			.filter_id = MOVE_SOURCE_FILTER_ID,
		},
	},
	.action_count = 1,
};

static obs_source_t *find_action_filter(const workflow_action_t *action)
{
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

static void execute_workflow(workflow_t *wf)
{
	if (!wf->enabled)
		return;

	blog(LOG_INFO, "[Move Workflow Phase 7] Executing workflow: \"%s\" (%zu action(s))",
	     wf->name, wf->action_count);

	for (size_t i = 0; i < wf->action_count; ++i) {
		workflow_action_t *action = &wf->actions[i];
		obs_source_t *filter = find_action_filter(action);

		if (!filter) {
			blog(LOG_WARNING,
			     "[Move Workflow Phase 7] Action %zu target not found: scene=\"%s\" filter=\"%s\" id=\"%s\"",
			     i + 1, action->scene_name, action->filter_name, action->filter_id);
			continue;
		}

		blog(LOG_INFO,
		     "[Move Workflow Phase 7] Action %zu: enabling scene=\"%s\" filter=\"%s\" id=\"%s\"",
		     i + 1, action->scene_name, action->filter_name, action->filter_id);

		obs_source_set_enabled(filter, false);
		obs_source_set_enabled(filter, true);
		obs_source_release(filter);
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

	blog(LOG_INFO, "[Move Workflow Phase 7] Loaded");
	blog(LOG_INFO, "[Move Workflow Phase 7] Workflow \"%s\" has %zu action(s)",
	     workflow.name, workflow.action_count);

	hotkey_id = obs_hotkey_register_frontend(
		"obs_move_workflow.test_workflow",
		"Move Workflow: Run Test Hotkey Workflow",
		workflow_hotkey_callback,
		&workflow);
	UNUSED_PARAMETER(hotkey_id);

	obs_frontend_add_tools_menu_item(
		"Move Workflow: Run Test Hotkey Workflow", menu_cb, NULL);

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[Move Workflow Phase 7] Unloaded");
}
