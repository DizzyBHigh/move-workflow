#include "workflow-trigger-dispatch.hpp"

#include "workflow-engine-service.h"

#include <obs.h>

#include <string>

namespace {
struct trigger_request {
    std::string workflow_id;
    std::string trigger_id;
};

static void run_trigger(void *opaque)
{
    auto *request = static_cast<trigger_request *>(opaque);
    if (!request)
        return;

    workflow_engine_service_trigger(request->workflow_id.c_str(),
                                    request->trigger_id.c_str());
    delete request;
}
}

void workflow_trigger_dispatch(const char *workflow_id, const char *trigger_id)
{
    if (!workflow_id || !*workflow_id || !trigger_id || !*trigger_id)
        return;

    auto *request = new trigger_request{workflow_id, trigger_id};
    obs_queue_task(OBS_TASK_UI, run_trigger, request, false);
}
