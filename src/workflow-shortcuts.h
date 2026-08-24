#pragma once

#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

void workflow_shortcuts_begin(workflow_t *workflow, const workflow_node_t *node);
bool workflow_shortcuts_accept(workflow_t *workflow, const char *source_id,
                               const char *target_id);
void workflow_shortcuts_cancel(void);

#ifdef __cplusplus
}
#endif
