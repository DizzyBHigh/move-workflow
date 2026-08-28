#include "workflow-engine-delay.h"

#include <obs.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

struct delay_context {
    std::chrono::steady_clock::time_point due;
    workflow_engine_delay_callback_t callback;
    void *data;
};

namespace {
std::mutex mutex;
std::condition_variable condition;
std::deque<delay_context *> pending;
std::thread worker;
bool shutting_down = false;

static void dispatch_delay(void *data)
{
    auto *context = static_cast<delay_context *>(data);
    if (!context)
        return;
    auto callback = context->callback;
    void *callback_data = context->data;
    delete context;
    if (callback)
        callback(callback_data);
}

static void run_scheduler()
{
    std::unique_lock<std::mutex> lock(mutex);
    while (!shutting_down) {
        if (pending.empty()) {
            condition.wait(lock, [] { return shutting_down || !pending.empty(); });
            continue;
        }

        auto *context = *std::min_element(
            pending.begin(), pending.end(), [](const auto *left, const auto *right) {
                return left->due < right->due;
            });
        condition.wait_until(lock, context->due);
        if (shutting_down)
            break;

        auto it = std::find(pending.begin(), pending.end(), context);
        if (it == pending.end())
            continue;
        pending.erase(it);
        lock.unlock();
        obs_queue_task(OBS_TASK_UI, dispatch_delay, context, false);
        lock.lock();
    }
}

static void ensure_worker()
{
    if (worker.joinable())
        return;
    shutting_down = false;
    worker = std::thread(run_scheduler);
}
} // namespace

bool workflow_engine_delay_start(uint64_t delay_ms,
                                 workflow_engine_delay_callback_t callback,
                                 void *data)
{
    if (!callback)
        return false;
    try {
        auto *context = new delay_context{
            std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms), callback, data};
        std::lock_guard<std::mutex> lock(mutex);
        ensure_worker();
        pending.push_back(context);
        condition.notify_one();
    } catch (...) {
        return false;
    }
    return true;
}

void workflow_engine_delay_shutdown(void)
{
    std::deque<delay_context *> cancelled;
    {
        std::lock_guard<std::mutex> lock(mutex);
        shutting_down = true;
        cancelled.swap(pending);
        condition.notify_all();
    }

    if (worker.joinable())
        worker.join();

    for (auto *context : cancelled) {
        if (context->callback)
            context->callback(context->data);
        delete context;
    }
}
