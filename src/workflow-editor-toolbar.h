#pragma once

#include "workflow-manager.h"
#include <functional>

class QWidget;

struct workflow_editor_toolbar_callbacks {
    std::function<void(const char *)> select_workflow;
    std::function<bool(const char *)> create_workflow;
    std::function<bool(const char *)> duplicate_workflow;
    std::function<bool()> delete_workflow;
    std::function<void()> rename_workflow;
    std::function<void(bool)> set_workflow_enabled;
    std::function<void()> import_workflow;
    std::function<void()> export_workflow;
    std::function<void()> zoom_out;
    std::function<void()> zoom_reset;
    std::function<void()> zoom_in;
    std::function<void()> fit;
    std::function<void()> close;
};

QWidget *create_workflow_editor_toolbar(QWidget *parent,
                                        workflow_manager_t *manager,
                                        workflow_editor_toolbar_callbacks callbacks);
void workflow_editor_toolbar_refresh(QWidget *toolbar);
