#include "workflow-engine-delay.h"

#include <thread>
#include <chrono>

struct delay_context {
    uint64_t delay_ms;
    workflow_engine_delay_callback_t callback;
    void *data;
};

static void run_delay(delay_context context)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(context.delay_ms));
    if (context.callback)
        context.callback(context.data);
}

bool workflow_engine_delay_start(uint64_t delay_ms,
                                 workflow_engine_delay_callback_t callback,
                                 void *data)
{
    if (!callback)
        return false;
    try {
        delay_context context{delay_ms, callback, data};
        std::thread(run_delay, context).detach();
    } catch (...) {
        return false;
    }
    return true;
}
