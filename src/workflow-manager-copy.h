#pragma once

#include "workflow-model.h"
#include "workflow-manager.h"

#ifdef __cplusplus
extern "C" {
#endif

workflow_t *workflow_manager_duplicate(workflow_manager_t *manager,
                                       const char *source_id,
                                       const char *new_id,
                                       const char *new_name);

#ifdef __cplusplus
}
#endif
