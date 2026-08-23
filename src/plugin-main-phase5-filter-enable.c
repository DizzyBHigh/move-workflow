/* Phase 5 sandbox test.
 *
 * LOCKED TEST TARGETS:
 *   scene  = obs-move-workflow test scene
 *   filters = Move Source - Left, Move Source - Center, Move Source - Right
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
#define MOVE_SOURCE_FILTER_ID "move_source_filter"

static const char *test_filter_names[] = {
	"Move Source - Left",
	"Move Source - Center",
	"Move Source - Right",
};

static obs_source_t *get_locked_filter(const char *filter_name)
{
	obs_source_t *scene_source = obs_get_source_by_name(TEST_SCENE_NAME);
	if (!scene_source)
		return NULL;

	obs_source_t *filter = obs_source_get_filter_by_name(scene_source, filter_name);
	obs_source_release(scene_source);

	if (!filter)
		return NULL;

	if (strcmp(obs_source_get_id(filter), MOVE_SOURCE_FILTER_ID) != 0) {
		obs_source_release(filter);
		return NULL;
	}

	return filter;
}

static void trigger_one(const char *filter_name)
{
	obs_source_t *filter = get_locked_filter(filter_name);
	if (!filter) {
		blog(LOG_WARNING,
		     "[Move Workflow Phase 5] TARGET NOT FOUND: scene=\"%s\" filter=\"%s\" id=\"%s\"",
		     TEST_SCENE_NAME, filter_name, MOVE_SOURCE_FILTER_ID);
		return;
	}

	blog(LOG_INFO,
	     "[Move Workflow Phase 5] TARGET LOCKED: scene=\"%s\" filter=\"%s\" id=\"%s\"",
	     TEST_SCENE_NAME, filter_name, MOVE_SOURCE_FILTER_ID);

	obs_source_set_enabled(filter, false);
	obs_source_set_enabled(filter, true);

	blog(LOG_INFO,
	     "[Move Workflow Phase 5] Toggled ONLY locked filter: scene=\"%s\" filter=\"%s\"",
	     TEST_SCENE_NAME, filter_name);

	obs_source_release(filter);
}

static void left_cb(void *data)
{
	UNUSED_PARAMETER(data);
	trigger_one(test_filter_names[0]);
}

static void center_cb(void *data)
{
	UNUSED_PARAMETER(data);
	trigger_one(test_filter_names[1]);
}

static void right_cb(void *data)
{
	UNUSED_PARAMETER(data);
	trigger_one(test_filter_names[2]);
}

static void run_all_cb(void *data)
{
	UNUSED_PARAMETER(data);
	trigger_one(test_filter_names[0]);
	trigger_one(test_filter_names[1]);
	trigger_one(test_filter_names[2]);
}

static void menu_cb(void *data)
{
	run_all_cb(data);
}

bool obs_module_load(void)
{
	blog(LOG_INFO,
	     "Move Workflow Phase 5 loaded; tests locked to scene=\"%s\" and exactly three Move filters",
	     TEST_SCENE_NAME);

	obs_frontend_add_tools_menu_item(
		"Move Workflow: Test ONLY Move Source - Left", left_cb, NULL);
	obs_frontend_add_tools_menu_item(
		"Move Workflow: Test ONLY Move Source - Center", center_cb, NULL);
	obs_frontend_add_tools_menu_item(
		"Move Workflow: Test ONLY Move Source - Right", right_cb, NULL);
	obs_frontend_add_tools_menu_item(
		"Move Workflow: Test ALL THREE Move Filters", menu_cb, NULL);

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "Move Workflow Phase 5 unloaded");
}
