#pragma once

#include <QString>

class QWidget;

namespace workflow_editor_active_runs {

QWidget *create(QWidget *parent = nullptr);
void set_workflow(QWidget *panel, const QString &workflowId);
void refresh(QWidget *panel);

}
