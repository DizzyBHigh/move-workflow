#pragma once

#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

struct workflow_engine;
void workflow_engine_service_set(struct workflow_engine *engine);
bool workflow_engine_service_test_node(workflow_t *workflow, const char *node_id);

#ifdef __cplusplus
}
#endif
