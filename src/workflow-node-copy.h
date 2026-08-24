#pragma once

#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

bool workflow_node_duplicate(workflow_t *workflow, const char *node_id,
                             const char *new_id, const char *new_name);

#ifdef __cplusplus
}
#endif
