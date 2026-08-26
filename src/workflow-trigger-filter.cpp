#include "workflow-trigger-filter.h"
#include "workflow-trigger-filter-ui.h"
#include "workflow-engine-service.h"
#include <obs.h>
#include <string>

namespace {
struct trigger_filter {
    obs_source_t *source = nullptr;
    bool enabled = false;
};

static const char *name(void *)
{
    return "Trigger Workflow";
}

static void video_tick(void *param, float)
{
    auto *data = static_cast<trigger_filter *>(param);
    if (!data || !data->source)
        return;

    const bool enabled = obs_source_enabled(data->source);
    if (enabled == data->enabled)
        return;

    data->enabled = enabled;
    if (!enabled)
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

    obs_source_set_enabled(data->source, false);
}

static void *create(obs_data_t *, obs_source_t *source)
{
    auto *data = new trigger_filter;
    data->source = source;
    data->enabled = obs_source_enabled(source);
    return data;
}

static void destroy(void *opaque)
{
    delete static_cast<trigger_filter *>(opaque);
}

static obs_source_info info = []() {
    obs_source_info value{};
    value.id = "move_workflow_trigger_filter";
    value.type = OBS_SOURCE_TYPE_FILTER;
    value.output_flags = OBS_SOURCE_VIDEO;
    value.get_name = name;
    value.create = create;
    value.destroy = destroy;
    value.video_tick = video_tick;
    value.get_properties = workflow_trigger_filter_properties;
    return value;
}();
}

void workflow_trigger_filter_register(void)
{
    obs_register_source(&info);
}
