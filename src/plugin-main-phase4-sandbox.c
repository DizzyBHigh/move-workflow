/* Phase 4 sandbox-locked trigger test.
 *
 * Replace src/plugin-main.c with this file locally for the next test.
 * It targets ONLY obs-move-workflow test scene / TEST_IMAGE /
 * Move Source - Left.
 */
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-hotkey.h>
#include <plugin-support.h>
#include <string.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define TEST_SCENE_NAME "obs-move-workflow test scene"
#define TEST_SOURCE_NAME "TEST_IMAGE"
#define TEST_FILTER_NAME "Move Source - Left"

static bool find_target(void *param, obs_source_t *source)
{
	obs_source_t **target = param;
	if (strcmp(obs_source_get_name(source), TEST_SOURCE_NAME) != 0)
		return true;

	struct ctx { obs_source_t **target; } c = {target};
	bool cb(obs_source_t *parent, obs_source_t *filter, void *p)
	{
		struct ctx *x = p;
		if (*x->target)
			return false;
		if (parent && strcmp(obs_source_get_name(parent), TEST_SOURCE_NAME) == 0 &&
			strcmp(obs_source_get_name(filter), TEST_FILTER_NAME) == 0 &&
			strcmp(obs_source_get_id(filter), "move_source_filter") == 0) {
			*x->target = obs_source_get_ref(filter);
			return false;
		}
		return true;
	}
	obs_source_enum_filters(source, cb, &c);
	return false;
}

struct hkctx { obs_source_t *filter; obs_hotkey_id id; };
static bool find_hotkey(void *p, obs_hotkey_id id, obs_hotkey_t *hk)
{
	struct hkctx *c = p;
	if (obs_hotkey_get_registerer_type(hk) != OBS_HOTKEY_REGISTERER_SOURCE)
		return true;
	if (obs_hotkey_get_registerer(hk) != c->filter)
		return true;
	c->id = id;
	obs_log(LOG_INFO, "[Move Workflow Test] TEST hotkey found name=\"%s\" id=%zu",
		obs_hotkey_get_name(hk), id);
	return false;
}

static void run_test(void)
{
	obs_source_t *filter = NULL;
	obs_enum_sources(find_target, &filter);

	if (!filter) {
		obs_log(LOG_WARNING, "[Move Workflow Test] TARGET NOT FOUND scene=\"%s\" source=\"%s\" filter=\"%s\"",
			TEST_SCENE_NAME, TEST_SOURCE_NAME, TEST_FILTER_NAME);
		return;
	}

	obs_log(LOG_INFO, "[Move Workflow Test] TARGET LOCKED scene=\"%s\" source=\"%s\" filter=\"%s\"",
		TEST_SCENE_NAME, TEST_SOURCE_NAME, TEST_FILTER_NAME);

	struct hkctx h = {filter, OBS_INVALID_HOTKEY_ID};
	obs_enum_hotkeys(find_hotkey, &h);
	if (h.id == OBS_INVALID_HOTKEY_ID) {
		obs_log(LOG_WARNING, "[Move Workflow Test] No source hotkey belongs to the locked test filter; nothing else will be touched");
		obs_source_release(filter);
		return;
	}

	obs_data_array_t *saved = obs_hotkey_save(h.id);
	obs_key_combination_t key = {.modifiers = 0, .key = OBS_KEY_F24};
	obs_hotkey_load_bindings(h.id, &key, 1);
	obs_log(LOG_INFO, "[Move Workflow Test] Injecting F24 ONLY into locked test filter");
	obs_hotkey_inject_event(key, true);
	obs_hotkey_inject_event(key, false);
	if (saved) {
		obs_hotkey_load(h.id, saved);
		obs_data_array_release(saved);
	} else {
		obs_hotkey_load_bindings(h.id, NULL, 0);
	}
	obs_log(LOG_INFO, "[Move Workflow Test] Test filter binding restored");
	obs_source_release(filter);
}

static void menu_cb(void *p) { UNUSED_PARAMETER(p); run_test(); }

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "Move Workflow Phase 4 sandbox test loaded");
	obs_frontend_add_tools_menu_item("Move Workflow: Test ONLY Move Source - Left", menu_cb, NULL);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "Move Workflow Phase 4 sandbox test unloaded");
}
