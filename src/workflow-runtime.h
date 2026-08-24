#pragma once

#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

workflow_t *workflow_runtime_test_workflow(void);
void workflow_runtime_execute_node_by_id(workflow_t *workflow, const char *node_id);
void workflow_runtime_test_duration(workflow_t *workflow);

#ifdef __cplusplus
}
#endif
