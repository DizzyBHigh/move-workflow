#include "workflow-action-executor.hpp"

#include "workflow-filter-instance.hpp"
#include "workflow-runtime.h"

namespace {
workflow_execution_mode execution_mode = workflow_execution_mode::temporary_instance;
}

void workflow_action_executor_set_mode(workflow_execution_mode mode)
{
	execution_mode = mode;
}

workflow_execution_mode workflow_action_executor_get_mode()
{
	return execution_mode;
}

bool workflow_action_executor_execute(NodeItem *node)
{
	if (!node)
		return false;

	switch (execution_mode) {
	case workflow_execution_mode::temporary_instance:
		return workflow_filter_instance_execute_node(node);
	case workflow_execution_mode::legacy_runtime:
		return workflow_runtime_execute_node_by_id(node->id.toUtf8().constData()) != 0;
	}

	return false;
}
