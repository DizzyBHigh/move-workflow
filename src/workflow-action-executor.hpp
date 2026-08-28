#pragma once

#include "workflow-model.h"

bool workflow_action_executor_execute(workflow_t *workflow,
                                      workflow_node_t *node);
