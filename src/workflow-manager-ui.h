#pragma once

#include "workflow-manager.h"

#include <functional>

class QWidget;

QWidget *create_workflow_manager_ui(
    workflow_manager_t *manager,
    QWidget *parent = nullptr,
    std::function<void(const char *)> selectionChanged = {});
