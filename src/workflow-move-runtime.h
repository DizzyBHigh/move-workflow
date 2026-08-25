#pragma once

#include <stdbool.h>
#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

bool workflow_move_runtime_trigger(workflow_t *workflow, workflow_node_t *node);

#ifdef __cplusplus
}
#endif
