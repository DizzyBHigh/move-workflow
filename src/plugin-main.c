/*
Move Workflow - Integration Test

Phase 2 discovers the four Exeldro Move filter types and reports them
separately. It still does not start, stop, or modify any filter.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
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
#include <plugin-support.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static void scan_filters(void);

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

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "Move Workflow integration test loaded (version %s)", PLUGIN_VERSION);

	obs_frontend_add_event_callback(frontend_event_callback, NULL);
	obs_frontend_add_tools_menu_item("Move Workflow: Scan Move Filters", scan_menu_callback, NULL);

	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(frontend_event_callback, NULL);
	obs_log(LOG_INFO, "Move Workflow integration test unloaded");
}
