#pragma once

#include "workflow-manager.h"
#include "workflow-model.h"

#include <functional>

class QWidget;

struct workflow_editor_sidebar_callbacks {
    std::function<void(const char *)> select_workflow;
    std::function<bool(const char *)> create_workflow;
    std::function<bool(const char *)> duplicate_workflow;
    std::function<bool()> delete_workflow;
    std::function<void()> rename_workflow;
    std::function<void(bool)> set_workflow_enabled;
    std::function<void(const char *)> add_node;
    std::function<void()> import_workflow;
    std::function<void()> export_workflow;
};

QWidget *create_workflow_editor_sidebar(
    workflow_manager_t *manager,
    QWidget *parent,
    workflow_editor_sidebar_callbacks callbacks);

void workflow_editor_sidebar_refresh(QWidget *sidebar);
