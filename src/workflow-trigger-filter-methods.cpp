#include "workflow-trigger-filter-methods.h"
#include "workflow-engine-service.h"
#include "workflow-model.h"
#include <obs-frontend-api.h>
#include <obs-hotkey.h>
#include <cstring>

namespace {
struct runtime {
	obs_source_t *watch_source = nullptr, *watch_filter = nullptr;
	signal_handler_t *source_handler = nullptr, *filter_handler = nullptr;
	obs_hotkey_id hotkey = OBS_INVALID_HOTKEY_ID;
	workflow_trigger_type_t type = WORKFLOW_TRIGGER_NONE;
	workflow_trigger_state_t state = WORKFLOW_TRIGGER_STATE_ENABLED;
	char workflow[WORKFLOW_MAX_NAME]{}, trigger[WORKFLOW_MAX_NAME]{};
	char source_uuid[WORKFLOW_MAX_NAME]{}, filter_uuid[WORKFLOW_MAX_NAME]{};
	char setting[WORKFLOW_MAX_NAME]{}, value[WORKFLOW_MAX_VALUE]{}, action[WORKFLOW_MAX_NAME]{};
};
static void fire(runtime *d) { if (d && d->workflow[0] && d->trigger[0]) workflow_engine_service_trigger(d->workflow, d->trigger); }
static void enabled(void *p, calldata_t *c) { auto *d = static_cast<runtime *>(p); if (d && calldata_bool(c, "enabled") == (d->state == WORKFLOW_TRIGGER_STATE_ENABLED)) fire(d); }
static void show(void *p, calldata_t *) { fire(static_cast<runtime *>(p)); }
static void mute(void *p, calldata_t *c) { auto *d = static_cast<runtime *>(p); if (d && calldata_bool(c, "muted") == (d->state == WORKFLOW_TRIGGER_STATE_ENABLED)) fire(d); }
static void update(void *p, calldata_t *)
{
	auto *d = static_cast<runtime *>(p); if (!d || !d->watch_source || !d->setting[0]) return;
	obs_data_t *s = obs_source_get_settings(d->watch_source); if (!s) return;
	const char *v = obs_data_get_string(s, d->setting); if (!std::strcmp(v ? v : "", d->value)) fire(d); obs_data_release(s);
}
static bool frontend_matches(runtime *d, enum obs_frontend_event e)
{
	if (!d) return false;
	if (d->type == WORKFLOW_TRIGGER_SCENE_CHANGE) return e == OBS_FRONTEND_EVENT_SCENE_CHANGED;
	if (d->type != WORKFLOW_TRIGGER_FRONTEND_ACTION) return false;
	struct P { const char *n; enum obs_frontend_event e; } p[] = {
		{"Streaming Start", OBS_FRONTEND_EVENT_STREAMING_STARTED}, {"Streaming Stop", OBS_FRONTEND_EVENT_STREAMING_STOPPED},
		{"Recording Start", OBS_FRONTEND_EVENT_RECORDING_STARTED}, {"Recording Stop", OBS_FRONTEND_EVENT_RECORDING_STOPPED},
		{"Recording Pause", OBS_FRONTEND_EVENT_RECORDING_PAUSED}, {"Recording Unpause", OBS_FRONTEND_EVENT_RECORDING_UNPAUSED},
		{"Replay Buffer Start", OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED}, {"Replay Buffer Stop", OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED},
		{"Replay Buffer Save", OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED}, {"Studio Mode Enable", OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED},
		{"Studio Mode Disable", OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED}, {"Virtual Camera Start", OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED},
		{"Virtual Camera Stop", OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED}};
	for (const auto &x : p) if (!std::strcmp(x.n, d->action) && x.e == e) return true;
	return false;
}
static void frontend(enum obs_frontend_event e, void *p) { auto *d = static_cast<runtime *>(p); if (frontend_matches(d, e)) fire(d); }
static void hotkey(void *p, obs_hotkey_id, obs_hotkey_t *, bool pressed) { if (pressed) fire(static_cast<runtime *>(p)); }
static obs_source_t *by_uuid(const char *u) { return u && u[0] ? obs_get_source_by_uuid(u) : nullptr; }
struct filter_lookup { const char *uuid; obs_source_t *filter; };
static void find_filter(obs_source_t *, obs_source_t *filter, void *param)
{
	auto *lookup = static_cast<filter_lookup *>(param);
	if (!lookup->filter && lookup->uuid && !std::strcmp(obs_source_get_uuid(filter), lookup->uuid)) lookup->filter = obs_source_get_ref(filter);
}
static obs_source_t *filter_by_uuid(obs_source_t *source, const char *uuid)
{
	if (!source || !uuid || !uuid[0]) return nullptr;
	filter_lookup lookup{uuid, nullptr}; obs_source_enum_filters(source, find_filter, &lookup); return lookup.filter;
}
static void disconnect(runtime *d)
{
	if (!d) return;
	if (d->source_handler) {
		signal_handler_disconnect(d->source_handler, "show", show, d); signal_handler_disconnect(d->source_handler, "hide", show, d);
		signal_handler_disconnect(d->source_handler, "mute", mute, d); signal_handler_disconnect(d->source_handler, "update", update, d);
	}
	if (d->filter_handler) signal_handler_disconnect(d->filter_handler, "enable", enabled, d);
	if (d->hotkey != OBS_INVALID_HOTKEY_ID) obs_hotkey_unregister(d->hotkey);
	obs_frontend_remove_event_callback(frontend, d);
	if (d->watch_filter) obs_source_release(d->watch_filter); if (d->watch_source) obs_source_release(d->watch_source);
	d->watch_filter = d->watch_source = nullptr; d->source_handler = d->filter_handler = nullptr; d->hotkey = OBS_INVALID_HOTKEY_ID;
}
}
void workflow_trigger_filter_methods_create(void **o, obs_source_t *, obs_data_t *s) { if (!o) return; *o = new runtime; workflow_trigger_filter_methods_update(o, nullptr, s); }
void workflow_trigger_filter_methods_destroy(void **o) { if (!o || !*o) return; auto *d = static_cast<runtime *>(*o); disconnect(d); delete d; *o = nullptr; }
void workflow_trigger_filter_methods_update(void **o, obs_source_t *, obs_data_t *s)
{
	if (!o || !*o || !s) return;
	auto *d = static_cast<runtime *>(*o); disconnect(d);
	d->type = (workflow_trigger_type_t)obs_data_get_int(s, "method"); d->state = (workflow_trigger_state_t)obs_data_get_int(s, "method_state");
	std::strncpy(d->workflow, obs_data_get_string(s, "workflow"), WORKFLOW_MAX_NAME - 1); std::strncpy(d->trigger, obs_data_get_string(s, "trigger"), WORKFLOW_MAX_NAME - 1);
	std::strncpy(d->source_uuid, obs_data_get_string(s, "method_source"), WORKFLOW_MAX_NAME - 1); std::strncpy(d->filter_uuid, obs_data_get_string(s, "method_filter"), WORKFLOW_MAX_NAME - 1);
	std::strncpy(d->action, obs_data_get_string(s, "method_frontend"), WORKFLOW_MAX_NAME - 1); if (!d->action[0]) std::strncpy(d->action, obs_data_get_string(s, "method_hotkey"), WORKFLOW_MAX_NAME - 1);
	if (d->type == WORKFLOW_TRIGGER_SETTING) { std::strncpy(d->setting, obs_data_get_string(s, "method_setting"), WORKFLOW_MAX_NAME - 1); std::strncpy(d->value, obs_data_get_string(s, "method_value"), WORKFLOW_MAX_VALUE - 1); }
	if (d->type == WORKFLOW_TRIGGER_SOURCE_VISIBILITY || d->type == WORKFLOW_TRIGGER_SOURCE_MUTE || d->type == WORKFLOW_TRIGGER_SETTING || d->type == WORKFLOW_TRIGGER_SOURCE_HOTKEY || d->type == WORKFLOW_TRIGGER_FILTER_ENABLE || d->type == WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK) d->watch_source = by_uuid(d->source_uuid);
	if (d->type == WORKFLOW_TRIGGER_FILTER_ENABLE && d->watch_source) d->watch_filter = filter_by_uuid(d->watch_source, d->filter_uuid);
	if (d->watch_source) d->source_handler = obs_source_get_signal_handler(d->watch_source); if (d->watch_filter) d->filter_handler = obs_source_get_signal_handler(d->watch_filter);
	if (d->type == WORKFLOW_TRIGGER_SOURCE_VISIBILITY && d->source_handler) signal_handler_connect(d->source_handler, d->state == WORKFLOW_TRIGGER_STATE_ENABLED ? "show" : "hide", show, d);
	if (d->type == WORKFLOW_TRIGGER_SOURCE_MUTE && d->source_handler) signal_handler_connect(d->source_handler, "mute", mute, d);
	if (d->type == WORKFLOW_TRIGGER_SETTING && d->source_handler) signal_handler_connect(d->source_handler, "update", update, d);
	if (d->filter_handler) signal_handler_connect(d->filter_handler, "enable", enabled, d);
	if (d->type == WORKFLOW_TRIGGER_FRONTEND_ACTION || d->type == WORKFLOW_TRIGGER_SCENE_CHANGE) obs_frontend_add_event_callback(frontend, d);
	if (d->type == WORKFLOW_TRIGGER_SOURCE_HOTKEY || d->type == WORKFLOW_TRIGGER_FRONTEND_HOTKEY) d->hotkey = d->watch_source ? obs_hotkey_register_source(d->watch_source, d->action, "Workflow Trigger", hotkey, d) : obs_hotkey_register_frontend(d->action, "Workflow Trigger", hotkey, d);
}
