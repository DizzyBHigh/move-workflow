/* Phase 5 sandbox test.
 *
 * This deliberately uses only OBS's public filter-enable mechanism.
 * Target is LOCKED to:
 *   scene  = obs-move-workflow test scene
 *   source = TEST_IMAGE
 *   filter = Move Source - Left
 *
 * It does not enumerate or modify any other filter.
 */
#include <obs-module.h>
#include <obs.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <string.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEST_SCENE_NAME "obs-move-workflow test scene"
#define TEST_SOURCE_NAME "TEST_IMAGE"
#define TEST_FILTER_NAME "Move Source - Left"
#define MOVE_SOURCE_FILTER_ID "move_source_filter"

static obs_source_t *get_locked_filter(void)
{
	obs_source_t *scene_source = obs_get_source_by_name(TEST_SCENE_NAME);
	if (!scene_source)
		return NULL;

	obs_scene_t *scene = obs_scene_from_source(scene_source);
	if (!scene) {
		obs_source_release(scene_source);
		return NULL;
	}

	obs_sceneitem_t *item = obs_scene_find_source(scene, TEST_SOURCE_NAME);
	if (!item) {
		obs_source_release(scene_source);
		return NULL;
	}

	obs_source_t *source = obs_sceneitem_get_source(item);
	if (!source) {
		obs_source_release(scene_source);
		return NULL;
	}

	obs_source_t *filter = obs_source_get_filter_by_name(source, TEST_FILTER_NAME);
	if (!filter || strcmp(obs_source_get_id(filter), MOVE_SOURCE_FILTER_ID) != 0) {
		if (filter)
			obs_source_release(filter);
		obs_source_release(scene_source);
		return NULL;
	}

	/* obs_source_get_filter_by_name() returns a referenced source. */
	obs_source_release(scene_source);
	return filter;
}

static void run_filter_enable_test(void)
{
	obs_source_t *filter = get_locked_filter();
	if (!filter) {
		blog(LOG_WARNING,
		     "[Move Workflow Phase 5] TARGET NOT FOUND; locked target is scene=\"%s\" source=\"%s\" filter=\"%s\" id=\"%s\"",
		     TEST_SCENE_NAME, TEST_SOURCE_NAME, TEST_FILTER_NAME, MOVE_SOURCE_FILTER_ID);
		return;
	}

	blog(LOG_INFO,
	     "[Move Workflow Phase 5] TARGET LOCKED: scene=\"%s\" source=\"%s\" filter=\"%s\"",
	     TEST_SCENE_NAME, TEST_SOURCE_NAME, TEST_FILTER_NAME);

	/*
	 * Exeldro's Move filter supports filter-enable as a native start trigger.
	 * We deliberately exercise that existing mechanism rather than touching
	 * hotkeys or calling private Exeldro functions.
	 */
	obs_source_set_enabled(filter, false);
	blog(LOG_INFO, "[Move Workflow Phase 5] Locked Move Source - Left -> disabled");

	obs_source_set_enabled(filter, true);
	blog(LOG_INFO, "[Move Workflow Phase 5] Locked Move Source - Left -> enabled; native enable trigger should fire");

	obs_source_release(filter);
}

static void menu_cb(void *data)
{
	UNUSED_PARAMETER(data);
	run_filter_enable_test();
}

bool obs_module_load(void)
{
	blog(LOG_INFO,
	     "Move Workflow Phase 5 loaded; test locked to obs-move-workflow test scene / TEST_IMAGE / Move Source - Left");
	obs_frontend_add_tools_menu_item("Move Workflow: Phase 5 - Trigger ONLY Move Source - Left", menu_cb, NULL);
	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "Move Workflow Phase 5 unloaded");
}
