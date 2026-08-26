#include "workflow-trigger-filter-instance.h"
#include <cstring>

namespace {
constexpr const char *FILTER_ID = "move_workflow_trigger_filter";
struct enum_context { workflow_trigger_filter_enum_cb callback; void *param; };
struct find_context { const char *uuid; obs_source_t *result; };
static void enum_filters(obs_source_t *source, obs_source_t *filter, void *opaque)
{
    auto *ctx = static_cast<enum_context *>(opaque);
    if (ctx && ctx->callback && workflow_trigger_filter_is_instance(filter)) ctx->callback(source, filter, ctx->param);
}
static bool enum_sources(void *opaque, obs_source_t *source)
{
    auto *ctx = static_cast<enum_context *>(opaque); if (source) obs_source_enum_filters(source, enum_filters, ctx); return true;
}
static void find_filter(obs_source_t *, obs_source_t *filter, void *opaque)
{
    auto *ctx = static_cast<find_context *>(opaque);
    if (!ctx->result && workflow_trigger_filter_is_instance(filter) && std::strcmp(obs_source_get_uuid(filter), ctx->uuid) == 0) ctx->result = obs_source_get_ref(filter);
}
}

bool workflow_trigger_filter_is_instance(obs_source_t *filter)
{
    return filter && std::strcmp(obs_source_get_id(filter), FILTER_ID) == 0;
}
void workflow_trigger_filter_enum_instances(workflow_trigger_filter_enum_cb callback, void *param)
{
    if (!callback) return; enum_context ctx{callback, param}; obs_enum_scenes(enum_sources, &ctx); obs_enum_sources(enum_sources, &ctx);
}
obs_source_t *workflow_trigger_filter_find(const char *source_uuid, const char *filter_uuid)
{
    if (!source_uuid || !filter_uuid) return nullptr; obs_source_t *source = obs_get_source_by_uuid(source_uuid); if (!source) return nullptr;
    find_context ctx{filter_uuid, nullptr}; obs_source_enum_filters(source, find_filter, &ctx); obs_source_release(source); return ctx.result;
}
bool workflow_trigger_filter_get_target(obs_source_t *filter, char *workflow_id, char *trigger_id)
{
    if (!filter || !workflow_trigger_filter_is_instance(filter)) return false; obs_data_t *settings = obs_source_get_settings(filter); if (!settings) return false;
    if (workflow_id) std::strncpy(workflow_id, obs_data_get_string(settings, "workflow"), WORKFLOW_MAX_NAME - 1);
    if (trigger_id) std::strncpy(trigger_id, obs_data_get_string(settings, "trigger"), WORKFLOW_MAX_NAME - 1);
    if (workflow_id) workflow_id[WORKFLOW_MAX_NAME - 1] = '\0'; if (trigger_id) trigger_id[WORKFLOW_MAX_NAME - 1] = '\0'; obs_data_release(settings); return true;
}
bool workflow_trigger_filter_set_target(obs_source_t *filter, const char *workflow_id, const char *trigger_id)
{
    if (!filter || !workflow_trigger_filter_is_instance(filter)) return false; obs_data_t *settings = obs_source_get_settings(filter); if (!settings) return false;
    obs_data_set_string(settings, "workflow", workflow_id ? workflow_id : ""); obs_data_set_string(settings, "trigger", trigger_id ? trigger_id : ""); obs_source_update(filter, settings); obs_data_release(settings); return true;
}
void workflow_trigger_filter_ref_set(workflow_trigger_filter_ref_t *ref, obs_source_t *source, obs_source_t *filter)
{
    if (!ref) return; std::memset(ref, 0, sizeof(*ref));
    if (source) std::strncpy(ref->source_uuid, obs_source_get_uuid(source), WORKFLOW_MAX_NAME - 1);
    if (filter) std::strncpy(ref->filter_uuid, obs_source_get_uuid(filter), WORKFLOW_MAX_NAME - 1);
}
bool workflow_trigger_filter_ref_matches(const workflow_trigger_filter_ref_t *ref, obs_source_t *source, obs_source_t *filter)
{
    return ref && source && filter && std::strcmp(ref->source_uuid, obs_source_get_uuid(source)) == 0 && std::strcmp(ref->filter_uuid, obs_source_get_uuid(filter)) == 0;
}
