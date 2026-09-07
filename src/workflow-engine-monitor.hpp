#pragma once

#include "workflow-engine-runs.h"
#include <QList>
#include <QString>

namespace workflow_engine_monitor {

struct RunInfo {
    uint64_t run_id = 0;
    QString workflow_id;
    QString workflow_name;
    QString current_node_id;
    bool running = false;
    bool waiting_for_shortcut = false;
};

QList<RunInfo> active_runs(workflow_engine_runs_t *runs,
                           const QString &workflow_id = QString());

bool stop_run(workflow_engine_runs_t *runs, uint64_t run_id);
size_t stop_workflow(workflow_engine_runs_t *runs, const QString &workflow_id);

}
