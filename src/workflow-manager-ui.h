#pragma once

#include "workflow-manager.h"
#include <functional>

class QWidget;

QWidget *create_workflow_manager_ui(
    workflow_manager_t *manager,
    QWidget *parent = nullptr,
    std::function<void(const char *)> selectionChanged = {},
    std::function<bool(const char *)> createWorkflow = {},
    std::function<bool(const char *)> duplicateWorkflow = {},
    std::function<bool()> deleteWorkflow = {},
    std::function<void()> stateChanged = {});
