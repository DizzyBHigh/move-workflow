#include "workflow-engine-monitor.hpp"
#include "workflow-engine.h"
#include "workflow-engine-runs.h"

namespace workflow_engine_monitor {

QList<RunInfo> active_runs(workflow_engine_t *engine, const QString &workflow_id)
{
    QList<RunInfo> result;
    const auto *runs = workflow_engine_runs_const(engine);
    if (!runs) return result;
    for (auto *run = workflow_engine_runs_head(const_cast<workflow_engine_runs_t *>(runs));
         run; run = workflow_engine_run_next(run)) {
        workflow_engine_run_info_t info{};
        if (!workflow_engine_run_get_info(run, &info) || !info.running) continue;
        if (!workflow_id.isEmpty() &&
            (!info.workflow_id || workflow_id != QString::fromUtf8(info.workflow_id))) continue;
        RunInfo item;
        item.run_id = info.run_id;
        if (info.workflow_id) item.workflow_id = QString::fromUtf8(info.workflow_id);
        if (info.workflow_name) item.workflow_name = QString::fromUtf8(info.workflow_name);
        if (info.current_node_id) item.current_node_id = QString::fromUtf8(info.current_node_id);
        item.running = info.running;
        item.waiting_for_shortcut = info.waiting_for_shortcut;
        result.append(item);
    }
    return result;
}

bool stop_run(workflow_engine_t *engine, uint64_t run_id)
{ return engine && workflow_engine_stop_run(engine, run_id); }

size_t stop_workflow(workflow_engine_t *engine, const QString &workflow_id)
{
    return engine && !workflow_id.isEmpty()
        ? (workflow_engine_stop_workflow(engine, workflow_id.toUtf8().constData()) ? 1 : 0) : 0;
}

}
