/*
Move Workflow - Phase 6

Phase 6 establishes the first real workflow-engine path:

    OBS hotkey -> workflow action -> enable a specific Move filter

The first action is deliberately locked to the existing integration-test scene
and Move Source - Left filter. This is a proof-of-engine path, not the final
configuration UI.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-hotkey.h>
#include <plugin-support.h>
#include <string.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEST_SCENE_NAME "obs-move-workflow test scene"
#define TEST_FILTER_NAME "Move Source - Left"
#define MOVE_SOURCE_FILTER_ID "move_source_filter"

static obs_hotkey_id workflow_hotkey_id = OBS_INVALID_HOTKEY_ID;

static obs_source_t *get_test_move_filter(void)
{
	obs_source_t *scene = obs_get_source_by_name(TEST_SCENE_NAME);
	if (!scene)
		return NULL;

	obs_source_t *filter = obs_source_get_filter_by_name(scene, TEST_FILTER_NAME);
	obs_source_release(scene);

	if (!filter)
		return NULL;

	const char *filter_id = obs_source_get_id(filter);
	if (!filter_id || strcmp(filter_id, MOVE_SOURCE_FILTER_ID) != 0) {
		obs_source_release(filter);
		return NULL;
	}

	return filter;
}

static bool trigger_test_move_filter(void)
{
	obs_source_t *filter = get_test_move_filter();
	if (!filter) {
		blog(LOG_WARNING,
		     "[Move Workflow Phase 6] TARGET NOT FOUND: scene=\"%s\" filter=\"%s\" id=\"%s\"",
		     TEST_SCENE_NAME, TEST_FILTER_NAME, MOVE_SOURCE_FILTER_ID);
		return false;
	}

	blog(LOG_INFO,
	     "[Move Workflow Phase 6] ACTION: enable Move filter scene=\"%s\" filter=\"%s\"",
	     TEST_SCENE_NAME, TEST_FILTER_NAME);

	/*
	 * Exeldro's Move filter reacts to its enabled state. We deliberately use
	 * the same mechanism that proved successful in Phase 5: force a clean
	 * disabled -> enabled transition and let Move perform the animation.
	 */
	obs_source_set_enabled(filter, false);
	obs_source_set_enabled(filter, true);

	obs_source_release(filter);
	return true;
}

static void workflow_hotkey_callback(void *data, obs_hotkey_id id, bool pressed)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(id);

	if (!pressed)
		return;

	blog(LOG_INFO, "[Move Workflow Phase 6] TRIGGER: hotkey");
	trigger_test_move_filter();
}

static void trigger_menu_callback(void *private_data)
{
	UNUSED_PARAMETER(private_data);

	blog(LOG_INFO, "[Move Workflow Phase 6] TRIGGER: Tools menu");
	trigger_test_move_filter();
}

bool obs_module_load(void)
{
	blog(LOG_INFO,
	     "Move Workflow Phase 6 loaded: hotkey -> enable Move Source - Left");

	workflow_hotkey_id = obs_hotkey_register_frontend(
		"move_workflow_phase6_test_hotkey",
		"Move Workflow: Trigger Move Source - Left",
		workflow_hotkey_callback,
		NULL);

	obs_frontend_add_tools_menu_item(
		"Move Workflow: Phase 6 - Trigger Move Source - Left",
		trigger_menu_callback,
		NULL);

	return true;
}

void obs_module_unload(void)
{
	if (workflow_hotkey_id != OBS_INVALID_HOTKEY_ID)
		obs_hotkey_unregister(workflow_hotkey_id);

	workflow_hotkey_id = OBS_INVALID_HOTKEY_ID;

	blog(LOG_INFO, "Move Workflow Phase 6 unloaded");
}
