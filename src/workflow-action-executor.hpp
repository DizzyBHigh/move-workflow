#pragma once

#include "workflow-model.h"

enum class workflow_execution_mode {
	temporary_instance,
	legacy_runtime,
};

void workflow_action_executor_set_mode(workflow_execution_mode mode);
workflow_execution_mode workflow_action_executor_get_mode();

bool workflow_action_executor_execute(workflow_t *workflow,
                                      workflow_node_t *node);
