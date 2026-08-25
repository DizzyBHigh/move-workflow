#include "workflow-engine-delay.h"

#include <obs.h>
#include <chrono>
#include <thread>

struct delay_context {
    uint64_t delay_ms;
    workflow_engine_delay_callback_t callback;
    void *data;
};

static void dispatch_delay(void *data)
{
    delay_context *context = (delay_context *)data;
    if (!context)
        return;
    workflow_engine_delay_callback_t callback = context->callback;
    void *callback_data = context->data;
    delete context;
    if (callback)
        callback(callback_data);
}

static void run_delay(delay_context *context)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(context->delay_ms));
    obs_queue_task(OBS_TASK_UI, dispatch_delay, context, false);
}

bool workflow_engine_delay_start(uint64_t delay_ms,
                                 workflow_engine_delay_callback_t callback,
                                 void *data)
{
    if (!callback)
        return false;
    try {
        delay_context *context = new delay_context{delay_ms, callback, data};
        std::thread(run_delay, context).detach();
    } catch (...) {
        return false;
    }
    return true;
}
