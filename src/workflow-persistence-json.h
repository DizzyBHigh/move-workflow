#pragma once

#include "workflow-manager.h"
#include <QJsonObject>

QJsonObject workflow_manager_to_json(const workflow_manager_t *manager);
bool workflow_manager_from_json(workflow_manager_t *manager, const QJsonObject &object);
