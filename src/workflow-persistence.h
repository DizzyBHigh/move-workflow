#pragma once

#include "workflow-manager.h"

#ifdef __cplusplus
extern "C" {
#endif

void workflow_persistence_init(void);
workflow_manager_t *workflow_persistence_manager(void);
bool workflow_persistence_sync(const workflow_manager_t *manager);
bool workflow_persistence_save(void);

#ifdef __cplusplus
}
#endif
