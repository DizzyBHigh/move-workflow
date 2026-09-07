#include "workflow-engine-monitor.hpp"

#include <cstring>

namespace workflow_engine_monitor {

QList<RunInfo> active_runs(workflow_engine_runs_t *runs,
                           const QString &workflow_id)
{
    QList<RunInfo> result;
    if (!runs) return result;

    for (workflow_engine_run_t *run = workflow_engine_runs_head(runs);
         run; run = workflow_engine_run_next(run)) {
        workflow_engine_run_info_t info{};
        if (!workflow_engine_run_get_info(run, &info) || !info.running)
            continue;
        if (!workflow_id.isEmpty() &&
            (!info.workflow_id || workflow_id != QString::fromUtf8(info.workflow_id)))
            continue;

        RunInfo item;
        item.run_id = info.run_id;
        if (info.workflow_id)
            item.workflow_id = QString::fromUtf8(info.workflow_id);
        if (info.workflow_name)
            item.workflow_name = QString::fromUtf8(info.workflow_name);
        if (info.current_node_id)
            item.current_node_id = QString::fromUtf8(info.current_node_id);
        item.running = info.running;
        item.waiting_for_shortcut = info.waiting_for_shortcut;
        result.append(item);
    }
    return result;
}

bool stop_run(workflow_engine_runs_t *runs, uint64_t run_id)
{
    workflow_engine_run_t *run = workflow_engine_runs_find(runs, run_id);
    if (!run) return false;
    workflow_engine_state_t *state = workflow_engine_run_state(run);
    if (!state || !workflow_engine_state_is_active(state)) return false;
    workflow_engine_state_stop(state);
    return true;
}

size_t stop_workflow(workflow_engine_runs_t *runs, const QString &workflow_id)
{
    if (workflow_id.isEmpty()) return 0;
    return workflow_engine_runs_stop_workflow(
        runs, workflow_id.toUtf8().constData());
}

}
