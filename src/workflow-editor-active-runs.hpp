#pragma once

#include <QString>
#include <functional>

class QWidget;
struct workflow_engine;

namespace workflow_editor_active_runs {

QWidget *create(QWidget *parent = nullptr, workflow_engine *engine = nullptr);
void set_workflow(QWidget *panel, const QString &workflowId);
void refresh(QWidget *panel);
void set_refresh_callback(QWidget *panel, std::function<void()> callback);

}
