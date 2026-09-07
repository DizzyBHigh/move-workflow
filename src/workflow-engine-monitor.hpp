#pragma once

#include "workflow-engine.h"
#include <QList>
#include <QString>

namespace workflow_engine_monitor {

struct RunInfo {
    uint64_t run_id = 0;
    QString workflow_id;
    QString workflow_name;
    QString current_node_id;
    QString current_node_name;
    QString phase;
    qint64 elapsed_ms = 0;
    qint64 duration_ms = 0;
    bool running = false;
    bool waiting_for_shortcut = false;
};

QList<RunInfo> active_runs(workflow_engine_t *engine,
                           const QString &workflow_id = QString());

bool stop_run(workflow_engine_t *engine, uint64_t run_id);
size_t stop_workflow(workflow_engine_t *engine, const QString &workflowId);
size_t stop_scope(workflow_engine_t *engine, const QString &workflowId);

}
