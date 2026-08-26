#include "workflow-trigger-filter.h"
#include "workflow-trigger-filter-ui.h"
#include "workflow-engine-service.h"
#include <obs.h>

namespace {
struct trigger_filter {
    obs_source_t *source = nullptr;
    bool enabled = false;
};

static const char *name(void *)
{
    return "Trigger Workflow";
}

static void enabled_signal(void *param, calldata_t *calldata)
{
    auto *data = static_cast<trigger_filter *>(param);
    if (!data)
        return;

    const bool enabled = calldata_bool(calldata, "enabled");
    if (enabled == data->enabled)
        return;

    data->enabled = enabled;
    if (!enabled)
        return;

    obs_data_t *settings = obs_source_get_settings(data->source);
    if (!settings)
        return;

    const char *workflow = obs_data_get_string(settings, "workflow");
    const char *trigger = obs_data_get_string(settings, "trigger");
    const bool valid_target = workflow && workflow[0] && trigger && trigger[0];
    const char *workflow_id = workflow ? bstrdup(workflow) : nullptr;
    const char *trigger_id = trigger ? bstrdup(trigger) : nullptr;
    obs_data_release(settings);

    // Reset the adapter before firing the workflow so it is immediately ready
    // for the next Move Filter Enable action.
    data->enabled = false;
    obs_source_set_enabled(data->source, false);

    if (valid_target)
        workflow_engine_service_trigger(workflow_id, trigger_id);

    bfree((void *)workflow_id);
    bfree((void *)trigger_id);
}

static void *create(obs_data_t *, obs_source_t *source)
{
    auto *data = new trigger_filter;
    data->source = source;
    data->enabled = obs_source_enabled(source);

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
