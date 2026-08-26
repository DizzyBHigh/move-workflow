#include "workflow-trigger-filter.h"
#include "workflow-trigger-filter-methods.h"
#include "workflow-trigger-filter-ui.h"
#include "workflow-engine-service.h"
#include "workflow-model.h"
#include <obs.h>
#include <cstring>

namespace {
struct trigger_filter { obs_source_t *source = nullptr; bool enabled = false; void *methods = nullptr; };
static const char *name(void *) { return "Trigger Workflow"; }
static void defaults(obs_data_t *settings) { obs_data_set_default_int(settings, "method", WORKFLOW_TRIGGER_NONE); obs_data_set_default_int(settings, "method_state", WORKFLOW_TRIGGER_STATE_ENABLED); }
static void enabled_signal(void *param, calldata_t *calldata)
{
    auto *data = static_cast<trigger_filter *>(param); if (!data) return;
    const bool enabled = calldata_bool(calldata, "enabled"); if (enabled == data->enabled) return;
    data->enabled = enabled; if (!enabled) return;
    char workflow[WORKFLOW_MAX_NAME]{}, trigger[WORKFLOW_MAX_NAME]{};
    obs_data_t *settings = obs_source_get_settings(data->source); if (!settings) return;
    std::strncpy(workflow, obs_data_get_string(settings, "workflow"), WORKFLOW_MAX_NAME - 1);
    std::strncpy(trigger, obs_data_get_string(settings, "trigger"), WORKFLOW_MAX_NAME - 1);
    obs_data_release(settings);
    if (workflow[0] && trigger[0]) workflow_engine_service_trigger(workflow, trigger);
}
static void *create(obs_data_t *settings, obs_source_t *source)
{
    auto *data = new trigger_filter; data->source = source; data->enabled = obs_source_enabled(source);
    workflow_trigger_filter_methods_create(&data->methods, source, settings);
    if (auto *handler = obs_source_get_signal_handler(source)) signal_handler_connect(handler, "enable", enabled_signal, data);
    return data;
}
static void destroy(void *opaque)
{
    auto *data = static_cast<trigger_filter *>(opaque); if (!data) return;
    if (auto *handler = obs_source_get_signal_handler(data->source)) signal_handler_disconnect(handler, "enable", enabled_signal, data);
    workflow_trigger_filter_methods_destroy(&data->methods); delete data;
}
static void update(void *opaque, obs_data_t *settings) { auto *data = static_cast<trigger_filter *>(opaque); if (data) workflow_trigger_filter_methods_update(&data->methods, data->source, settings); }
static obs_source_info info = [](){ obs_source_info value{}; value.id="move_workflow_trigger_filter"; value.type=OBS_SOURCE_TYPE_FILTER; value.output_flags=OBS_SOURCE_VIDEO; value.get_name=name; value.get_defaults=defaults; value.create=create; value.destroy=destroy; value.update=update; value.get_properties=workflow_trigger_filter_properties; return value; }();
}
void workflow_trigger_filter_register(void) { obs_register_source(&info); }
