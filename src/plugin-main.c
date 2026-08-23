/*
Move Workflow - Integration Test

Phase 3 tests whether an existing Exeldro Move filter can be invoked through
OBS's own hotkey system without simulating a physical keyboard. The test
temporarily assigns an OBS key combination to the target Move filter, injects
that combination through libobs, then restores the original binding.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-hotkey.h>
#include <plugin-support.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static void scan_filters(void);
static void trigger_first_move_filter(void);

static const char *move_filter_type(const char *filter_id)
{
	if (!filter_id)
		return NULL;

	if (strcmp(filter_id, "move_source_filter") == 0)
		return "Move Source";

	if (strcmp(filter_id, "move_value_filter") == 0)
		return "Move Value";

	if (strcmp(filter_id, "move_action_filter") == 0)
		return "Move Action";

	if (strcmp(filter_id, "move_source_swap_filter") == 0)
		return "Move Source Swap";

	return NULL;
}

static void filter_enum_callback(obs_source_t *parent, obs_source_t *filter, void *param)
{
	UNUSED_PARAMETER(param);

	const char *parent_name = obs_source_get_name(parent);
	const char *filter_name = obs_source_get_name(filter);
	const char *filter_id = obs_source_get_id(filter);
	const char *move_type = move_filter_type(filter_id);

	if (!move_type)
		return;

	obs_log(LOG_INFO,
		"[Move Workflow Test] MOVE_FILTER type=\"%s\" parent=\"%s\" name=\"%s\" id=\"%s\"",
		move_type,
		parent_name ? parent_name : "<unnamed>",
		filter_name ? filter_name : "<unnamed>",
		filter_id ? filter_id : "<null>");

	obs_data_t *settings = obs_source_get_settings(filter);
	if (settings) {
		const char *json = obs_data_get_json_pretty(settings);
		obs_log(LOG_INFO, "[Move Workflow Test] MOVE_SETTINGS type=\"%s\" %s",
			move_type, json ? json : "<null>");
		obs_data_release(settings);
	}
}

static bool source_enum_callback(void *param, obs_source_t *source)
{
	UNUSED_PARAMETER(param);

	obs_source_enum_filters(source, filter_enum_callback, NULL);
	return true;
}

static void scan_filters(void)
{
	obs_log(LOG_INFO, "[Move Workflow Test] ===== BEGIN MOVE FILTER SCAN =====");
	obs_enum_sources(source_enum_callback, NULL);
	obs_log(LOG_INFO, "[Move Workflow Test] ===== END MOVE FILTER SCAN =====");
}

struct find_move_context {
	obs_source_t *filter;
};

static bool find_first_move_filter_callback(obs_source_t *parent, obs_source_t *filter, void *param)
{
	UNUSED_PARAMETER(parent);

	struct find_move_context *ctx = param;
	if (ctx->filter)
		return false;

	if (move_filter_type(obs_source_get_id(filter))) {
		ctx->filter = obs_source_get_ref(filter);
		return false;
	}

	return true;
}

static bool find_first_move_source_callback(void *param, obs_source_t *source)
{
	struct find_move_context *ctx = param;
	if (ctx->filter)
		return false;

	obs_source_enum_filters(source, find_first_move_filter_callback, ctx);
	return ctx->filter == NULL;
}

struct hotkey_find_context {
	obs_source_t *source;
	obs_hotkey_id id;
};

static bool find_move_hotkey_callback(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey)
{
	struct hotkey_find_context *state = data;
	if (state->id != OBS_INVALID_HOTKEY_ID)
		return false;

	if (obs_hotkey_get_registerer_type(hotkey) != OBS_HOTKEY_REGISTERER_SOURCE)
		return true;

	if (obs_hotkey_get_registerer(hotkey) != state->source)
		return true;

	const char *name = obs_hotkey_get_name(hotkey);
	if (name && strcmp(name, obs_source_get_name(state->source)) == 0) {
		state->id = id;
		return false;
	}

	return true;
}

static void trigger_first_move_filter(void)
{
	struct find_move_context ctx = {0};
	obs_enum_sources(find_first_move_source_callback, &ctx);

	if (!ctx.filter) {
		obs_log(LOG_WARNING, "[Move Workflow Test] No Exeldro Move filter found to trigger");
		return;
	}

	const char *filter_name = obs_source_get_name(ctx.filter);
	const char *filter_id = obs_source_get_id(ctx.filter);
	obs_log(LOG_INFO,
		"[Move Workflow Test] HOTKEY TEST target=\"%s\" id=\"%s\"",
		filter_name ? filter_name : "<unnamed>",
		filter_id ? filter_id : "<null>");

	struct hotkey_find_context find_ctx = {
		.source = ctx.filter,
		.id = OBS_INVALID_HOTKEY_ID,
	};

	obs_enum_hotkeys(find_move_hotkey_callback, &find_ctx);

	if (find_ctx.id == OBS_INVALID_HOTKEY_ID) {
		obs_log(LOG_WARNING,
			"[Move Workflow Test] Could not find an OBS hotkey registered for \"%s\"",
			filter_name ? filter_name : "<unnamed>");
		obs_source_release(ctx.filter);
		return;
	}

	obs_data_array_t *original_bindings = obs_hotkey_save(find_ctx.id);

	/* F24 is used only for this experiment and is restored immediately. */
	obs_key_combination_t test_key = {
		.modifiers = 0,
		.key = OBS_KEY_F24,
	};

	obs_hotkey_load_bindings(find_ctx.id, &test_key, 1);
	obs_log(LOG_INFO, "[Move Workflow Test] Injecting temporary F24 binding for hotkey id=%zu", find_ctx.id);
	obs_hotkey_inject_event(test_key, true);
	obs_hotkey_inject_event(test_key, false);

	if (original_bindings) {
		obs_hotkey_load(find_ctx.id, original_bindings);
		obs_data_array_release(original_bindings);
	} else {
		obs_hotkey_load_bindings(find_ctx.id, NULL, 0);
	}

	obs_log(LOG_INFO, "[Move Workflow Test] Restored original hotkey binding");
	obs_source_release(ctx.filter);
}

static void frontend_event_callback(enum obs_frontend_event event, void *private_data)
{
	UNUSED_PARAMETER(private_data);

	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING)
		scan_filters();
}

static void scan_menu_callback(void *private_data)
{
	UNUSED_PARAMETER(private_data);
	scan_filters();
}

static void trigger_menu_callback(void *private_data)
{
	UNUSED_PARAMETER(private_data);
	trigger_first_move_filter();
}

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "Move Workflow integration test loaded (version %s)", PLUGIN_VERSION);

	obs_frontend_add_event_callback(frontend_event_callback, NULL);
	obs_frontend_add_tools_menu_item("Move Workflow: Scan Move Filters", scan_menu_callback, NULL);
	obs_frontend_add_tools_menu_item("Move Workflow: Test Trigger First Move Filter", trigger_menu_callback, NULL);

	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(frontend_event_callback, NULL);
	obs_log(LOG_INFO, "Move Workflow integration test unloaded");
}
