/* Phase 5 sandbox test.
 *
 * LOCKED TEST TARGET:
 *   scene  = obs-move-workflow test scene
 *   filter = Move Source - Left
 *
 * The Move filters are attached directly to the scene source. TEST_IMAGE is
 * deliberately NOT searched and no other scene/source/filter is enumerated.
 */
#include <obs-module.h>
#include <obs.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <string.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEST_SCENE_NAME "obs-move-workflow test scene"
#define TEST_FILTER_NAME "Move Source - Left"
#define MOVE_SOURCE_FILTER_ID "move_source_filter"

static obs_source_t *get_locked_filter(void)
{
	obs_source_t *scene_source = obs_get_source_by_name(TEST_SCENE_NAME);
	if (!scene_source)
		return NULL;

	obs_source_t *filter = obs_source_get_filter_by_name(scene_source, TEST_FILTER_NAME);
	obs_source_release(scene_source);

	if (!filter)
		return NULL;

	if (strcmp(obs_source_get_id(filter), MOVE_SOURCE_FILTER_ID) != 0) {
		obs_source_release(filter);
		return NULL;
	}

	return filter;
}

static void run_filter_enable_test(void *data)
{
	UNUSED_PARAMETER(data);

	obs_source_t *filter = get_locked_filter();
	if (!filter) {
		blog(LOG_WARNING,
		     "[Move Workflow Phase 5] TARGET NOT FOUND; locked target is scene=\"%s\" filter=\"%s\" id=\"%s\"",
		     TEST_SCENE_NAME, TEST_FILTER_NAME, MOVE_SOURCE_FILTER_ID);
		return;
	}

	blog(LOG_INFO,
	     "[Move Workflow Phase 5] TARGET LOCKED: scene=\"%s\" filter=\"%s\" id=\"%s\"",
	     TEST_SCENE_NAME, TEST_FILTER_NAME, MOVE_SOURCE_FILTER_ID);

	obs_source_set_enabled(filter, false);
	blog(LOG_INFO, "[Move Workflow Phase 5] Locked Move Source - Left -> disabled");

	obs_source_set_enabled(filter, true);
	blog(LOG_INFO, "[Move Workflow Phase 5] Locked Move Source - Left -> enabled; native enable trigger should fire");

	obs_source_release(filter);
}

static void menu_cb(void *data)
{
	run_filter_enable_test(data);
}

bool obs_module_load(void)
{
	blog(LOG_INFO,
	     "Move Workflow Phase 5 loaded; test locked to scene=\"%s\" filter=\"%s\"",
	     TEST_SCENE_NAME, TEST_FILTER_NAME);

	obs_frontend_add_tools_menu_item(
		"Move Workflow: Phase 5 - Trigger ONLY Move Source - Left", menu_cb, NULL);

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "Move Workflow Phase 5 unloaded");
}
