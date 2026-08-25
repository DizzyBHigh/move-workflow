#pragma once

#include "workflow-manager.h"

#ifdef __cplusplus
extern "C" {
#endif

bool workflow_import_file(workflow_manager_t *manager, const char *path);

#ifdef __cplusplus
}
#endif
