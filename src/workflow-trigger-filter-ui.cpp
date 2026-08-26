#include "workflow-trigger-filter-ui.h"
#include "workflow-model.h"
#include "workflow-persistence.h"
#include <obs.h>

namespace {
static void add_sources(obs_property_t *p)
{
    obs_enum_scenes([](void *d, obs_source_t *s) { auto *p = static_cast<obs_property_t *>(d); obs_property_list_add_string(p, obs_source_get_name(s), obs_source_get_uuid(s)); return true; }, p);
    obs_enum_sources([](void *d, obs_source_t *s) { auto *p = static_cast<obs_property_t *>(d); obs_property_list_add_string(p, obs_source_get_name(s), obs_source_get_uuid(s)); return true; }, p);
}
static void add_frontend_actions(obs_property_t *p)
{
    const char *actions[] = {"None", "Streaming Start", "Streaming Stop", "Recording Start", "Recording Stop", "Recording Pause", "Recording Unpause", "Virtual Camera Start", "Virtual Camera Stop", "Replay Buffer Start", "Replay Buffer Stop", "Replay Buffer Save", "Studio Mode Enable", "Studio Mode Disable", "Take Screenshot"};
    for (const char *a : actions) obs_property_list_add_string(p, a, a);
}
static void add_state(obs_properties_t *props)
{
    auto *p = obs_properties_add_list(props, "method_state", "Enable", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(p, "Enable", WORKFLOW_TRIGGER_STATE_ENABLED);
    obs_property_list_add_int(p, "Disable", WORKFLOW_TRIGGER_STATE_DISABLED);
}
static obs_source_t *find_source(const char *value)
{
    if (!value || !value[0]) return nullptr;
    obs_source_t *source = obs_get_source_by_uuid(value);
    if (!source) source = obs_get_source_by_name(value);
    return source;
}
static void fill_filters(obs_properties_t *props, obs_data_t *s)
{
    auto *p = obs_properties_get(props, "method_filter");
    if (!p) return;
    const char *wanted = obs_data_get_string(s, "method_filter");
    obs_property_list_clear(p);
    obs_source_t *src = find_source(obs_data_get_string(s, "method_source"));
    if (!src) return;
    obs_source_enum_filters(src, [](obs_source_t *, obs_source_t *f, void *d) {
        auto *p = static_cast<obs_property_t *>(d);
        obs_property_list_add_string(p, obs_source_get_name(f), obs_source_get_uuid(f));
    }, p);
    if (wanted && wanted[0]) obs_data_set_string(s, "method_filter", wanted);
    obs_source_release(src);
}
static bool source_modified(obs_properties_t *props, obs_property_t *, obs_data_t *s)
{
    fill_filters(props, s);
    return true;
}
static bool workflow_modified(obs_properties_t *props, obs_property_t *, obs_data_t *s)
{
    auto *m = workflow_persistence_manager();
    auto *w = m ? workflow_manager_find(m, obs_data_get_string(s, "workflow")) : nullptr;
    auto *p = obs_properties_get(props, "trigger");
    if (!p) return true;
    obs_property_list_clear(p);
    if (w) for (size_t i = 0; i < w->node_count; ++i)
        if (w->nodes[i].type == WORKFLOW_NODE_TRIGGER)
            obs_property_list_add_string(p, w->nodes[i].name, w->nodes[i].id);
    return true;
}
static void add_methods(obs_property_t *p)
{
    obs_property_list_add_int(p, "None", WORKFLOW_TRIGGER_NONE);
    obs_property_list_add_int(p, "Frontend Action", WORKFLOW_TRIGGER_FRONTEND_ACTION);
    obs_property_list_add_int(p, "Source Visibility", WORKFLOW_TRIGGER_SOURCE_VISIBILITY);
    obs_property_list_add_int(p, "Source Mute", WORKFLOW_TRIGGER_SOURCE_MUTE);
    obs_property_list_add_int(p, "Source Audio Track", WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK);
    obs_property_list_add_int(p, "Source Hotkey", WORKFLOW_TRIGGER_SOURCE_HOTKEY);
    obs_property_list_add_int(p, "Filter Enable", WORKFLOW_TRIGGER_FILTER_ENABLE);
    obs_property_list_add_int(p, "Frontend Hotkey", WORKFLOW_TRIGGER_FRONTEND_HOTKEY);
    obs_property_list_add_int(p, "Setting", WORKFLOW_TRIGGER_SETTING);
    obs_property_list_add_int(p, "UDP packet", WORKFLOW_TRIGGER_UDP_PACKET);
    obs_property_list_add_int(p, "Websocket Request", WORKFLOW_TRIGGER_WEBSOCKET_REQUEST);
    obs_property_list_add_int(p, "Websocket Event", WORKFLOW_TRIGGER_WEBSOCKET_EVENT);
    obs_property_list_add_int(p, "Scene Change", WORKFLOW_TRIGGER_SCENE_CHANGE);
}
static void remove_method_fields(obs_properties_t *p)
{
    const char *names[] = {"method_canvas", "method_scene", "method_source", "method_filter", "method_enable", "method_audio_track", "method_hotkey", "method_setting", "method_value", "method_udp_host", "method_udp_port", "method_udp_packet", "method_request", "method_event", "method_data", "method_frontend", "method_state"};
    for (const char *n : names) obs_properties_remove_by_name(p, n);
}
static obs_property_t *add_list(obs_properties_t *p, const char *name, const char *label)
{
    return obs_properties_add_list(p, name, label, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
}
static obs_property_t *add_searchable_list(obs_properties_t *p, const char *name, const char *label)
{
    return obs_properties_add_list(p, name, label, OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
}
static bool method_modified(obs_properties_t *p, obs_property_t *, obs_data_t *s)
{
    remove_method_fields(p);
    auto t = (workflow_trigger_type_t)obs_data_get_int(s, "method");
    if (t == WORKFLOW_TRIGGER_FRONTEND_ACTION) { auto *q = add_list(p, "method_frontend", "Frontend Action"); add_frontend_actions(q); }
    if (t == WORKFLOW_TRIGGER_SOURCE_VISIBILITY) { add_list(p, "method_canvas", "Canvas"); add_list(p, "method_scene", "Scene"); auto *q = add_searchable_list(p, "method_source", "Source"); add_sources(q); add_state(p); }
    if (t == WORKFLOW_TRIGGER_SOURCE_MUTE) { auto *q = add_searchable_list(p, "method_source", "Source"); add_sources(q); add_state(p); }
    if (t == WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK) { auto *q = add_searchable_list(p, "method_source", "Source"); add_sources(q); q = add_list(p, "method_audio_track", "Audio Track"); const char *tracks[] = {"None", "1", "2", "3", "4", "5", "6"}; for (int i = 0; i <= 6; ++i) obs_property_list_add_int(q, tracks[i], i); add_state(p); }
    if (t == WORKFLOW_TRIGGER_SOURCE_HOTKEY) { auto *q = add_searchable_list(p, "method_source", "Source"); add_sources(q); add_list(p, "method_hotkey", "Hotkey"); }
    if (t == WORKFLOW_TRIGGER_FILTER_ENABLE) { auto *q = add_searchable_list(p, "method_source", "Source"); add_sources(q); auto *f = add_searchable_list(p, "method_filter", "Filter"); (void)f; obs_property_set_modified_callback(q, source_modified); fill_filters(p, s); add_state(p); }
    if (t == WORKFLOW_TRIGGER_FRONTEND_HOTKEY) add_list(p, "method_hotkey", "Hotkey");
    if (t == WORKFLOW_TRIGGER_SETTING) { auto *q = add_searchable_list(p, "method_source", "Source"); add_sources(q); add_searchable_list(p, "method_filter", "Filter"); obs_property_set_modified_callback(q, source_modified); fill_filters(p, s); add_list(p, "method_setting", "Setting"); obs_properties_add_text(p, "method_value", "Expected Value", OBS_TEXT_DEFAULT); }
    if (t == WORKFLOW_TRIGGER_UDP_PACKET) { obs_properties_add_text(p, "method_udp_host", "UDP host", OBS_TEXT_DEFAULT); obs_properties_add_int(p, "method_udp_port", "UDP port", 1, 65535, 1); obs_properties_add_text(p, "method_udp_packet", "UDP packet", OBS_TEXT_DEFAULT); }
    if (t == WORKFLOW_TRIGGER_WEBSOCKET_REQUEST) { obs_properties_add_text(p, "method_request", "Websocket Request", OBS_TEXT_DEFAULT); obs_properties_add_text(p, "method_data", "Data", OBS_TEXT_MULTILINE); }
    if (t == WORKFLOW_TRIGGER_WEBSOCKET_EVENT) { obs_properties_add_text(p, "method_event", "Websocket Event", OBS_TEXT_DEFAULT); obs_properties_add_text(p, "method_data", "Data", OBS_TEXT_MULTILINE); }
    return true;
}
}
obs_properties_t *workflow_trigger_filter_properties(void *)
{
    auto *p = obs_properties_create();
    auto *w = obs_properties_add_list(p, "workflow", "Workflow", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    auto *t = obs_properties_add_list(p, "trigger", "Workflow Trigger", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(t, "Select a workflow first", "");
    obs_property_set_modified_callback(w, workflow_modified);
    if (auto *m = workflow_persistence_manager()) for (size_t i = 0; i < m->workflow_count; ++i) obs_property_list_add_string(w, m->workflows[i].name, m->workflows[i].id);
    auto *method = obs_properties_add_list(p, "method", "Trigger", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    add_methods(method); obs_property_set_modified_callback(method, method_modified);
    return p;
}
