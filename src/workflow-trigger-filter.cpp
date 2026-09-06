#include "workflow-trigger-filter.h"
#include "workflow-trigger-filter-ui.h"
#include "workflow-engine-service.h"
#include "workflow-debug.h"
#include <obs.h>
#include <string>

namespace {
struct trigger_filter {
    obs_source_t *source = nullptr;
    bool enabled = false;
};

struct trigger_request {
    std::string workflow;
    std::string trigger;
};

static const char *name(void *)
{
    return "Trigger Workflow";
}

static void run_trigger(void *param)
{
    auto *request = static_cast<trigger_request *>(param);
    if (!request)
        return;

    workflow_debug_log("Trigger Workflow: queued callback begin workflow='%s' trigger='%s'",
                       request->workflow.c_str(), request->trigger.c_str());
    const bool result = workflow_engine_service_trigger(request->workflow.c_str(),
                                                        request->trigger.c_str());
    workflow_debug_log("Trigger Workflow: queued callback end result=%d workflow='%s' trigger='%s'",
                       result, request->workflow.c_str(), request->trigger.c_str());
    delete request;
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
    workflow_debug_log("Trigger Workflow: enabled transition source='%s' enabled=%d",
                       obs_source_get_name(data->source), enabled);
    if (!enabled)
        return;

    obs_data_t *settings = obs_source_get_settings(data->source);
    if (!settings)
        return;

    const std::string workflow = obs_data_get_string(settings, "workflow");
    const std::string trigger = obs_data_get_string(settings, "trigger");
    const bool valid_target = !workflow.empty() && !trigger.empty();
    workflow_debug_log("Trigger Workflow: target workflow='%s' trigger='%s' valid=%d",
                       workflow.c_str(), trigger.c_str(), valid_target);
    obs_data_release(settings);

    if (valid_target) {
        auto *request = new trigger_request{workflow, trigger};
        workflow_debug_log("Trigger Workflow: queueing UI trigger workflow='%s' trigger='%s' source_enabled=%d",
                           workflow.c_str(), trigger.c_str(), obs_source_enabled(data->source));
        obs_queue_task(OBS_TASK_UI, run_trigger, request, false);
        workflow_debug_log("Trigger Workflow: UI trigger queued; disabling source='%s'",
                           obs_source_get_name(data->source));
    }

    obs_source_set_enabled(data->source, false);
    workflow_debug_log("Trigger Workflow: source disabled source='%s' enabled_now=%d",
                       obs_source_get_name(data->source), obs_source_enabled(data->source));
}

static void *create(obs_data_t *, obs_source_t *source)
{
    auto *data = new trigger_filter;
    data->source = source;
    data->enabled = obs_source_enabled(source);
    workflow_debug_log("Trigger Workflow: instance created source='%s' enabled=%d",
                       source ? obs_source_get_name(source) : "", data->enabled);
    return data;
}

static void destroy(void *opaque)
{
    auto *data = static_cast<trigger_filter *>(opaque);
    if (data && data->source)
        workflow_debug_log("Trigger Workflow: instance destroyed source='%s' enabled=%d",
                           obs_source_get_name(data->source), obs_source_enabled(data->source));
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
    value.video_tick = video_tick;
    value.get_properties = workflow_trigger_filter_properties;
    return value;
}();
}

void workflow_trigger_filter_register(void)
{
    obs_register_source(&info);
}
