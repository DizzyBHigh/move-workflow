#include "workflow-trigger-filter.h"
#include "workflow-trigger-filter-ui.h"
#include "workflow-engine-service.h"
#include <obs.h>
#include <string>

namespace {
struct trigger_filter {
    obs_source_t *source = nullptr;
};

static const char *name(void *)
{
    return "Trigger Workflow";
}

static void disable_source(void *param)
{
    auto *source = static_cast<obs_source_t *>(param);
    if (!source)
        return;

    obs_source_set_enabled(source, false);
    obs_source_release(source);
}

static void enabled_signal(void *param, calldata_t *calldata)
{
    auto *data = static_cast<trigger_filter *>(param);
    if (!data || !data->source || !calldata_bool(calldata, "enabled"))
        return;

    obs_data_t *settings = obs_source_get_settings(data->source);
    if (!settings)
        return;

    const std::string workflow = obs_data_get_string(settings, "workflow");
    const std::string trigger = obs_data_get_string(settings, "trigger");
    const bool valid_target = !workflow.empty() && !trigger.empty();
    obs_data_release(settings);

    if (valid_target)
        workflow_engine_service_trigger(workflow.c_str(), trigger.c_str());

    // Keep the source alive until the deferred reset has completed. OBS owns
    // the authoritative enabled state; the filter does not mirror it locally.
    obs_source_addref(data->source);
    obs_queue_task(OBS_TASK_UI, disable_source, data->source, false);
}

static void *create(obs_data_t *, obs_source_t *source)
{
    auto *data = new trigger_filter;
    data->source = source;

    if (auto *handler = obs_source_get_signal_handler(source))
        signal_handler_connect(handler, "enable", enabled_signal, data);

    return data;
}

static void destroy(void *opaque)
{
    auto *data = static_cast<trigger_filter *>(opaque);
    if (!data)
        return;

    if (auto *handler = obs_source_get_signal_handler(data->source))
        signal_handler_disconnect(handler, "enable", enabled_signal, data);

    delete data;
}

static obs_source_info info = []() {
    obs_source_info value{};
    value.id = "move_workflow_trigger_filter";
    value.type = OBS_SOURCE_TYPE_FILTER;
    value.output_flags = OBS_SOURCE_VIDEO;
    value.get_name = name;
    value.create = create;
    value.destroy = destroy;
    value.get_properties = workflow_trigger_filter_properties;
    return value;
}();
}

void workflow_trigger_filter_register(void)
{
    obs_register_source(&info);
}
