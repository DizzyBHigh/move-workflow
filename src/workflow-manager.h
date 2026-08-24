#pragma once

#include "workflow-model.h"

#define WORKFLOW_MANAGER_MAX_WORKFLOWS 32

typedef struct workflow_manager {
    workflow_t workflows[WORKFLOW_MANAGER_MAX_WORKFLOWS];
    size_t workflow_count;
    char selected_workflow_id[WORKFLOW_MAX_NAME];
} workflow_manager_t;

#ifdef __cplusplus
extern "C" {
#endif

void workflow_manager_init(workflow_manager_t *manager);
workflow_t *workflow_manager_create(workflow_manager_t *manager,
                                    const char *id, const char *name);
bool workflow_manager_remove(workflow_manager_t *manager, const char *id);
workflow_t *workflow_manager_find(workflow_manager_t *manager, const char *id);
const workflow_t *workflow_manager_find_const(const workflow_manager_t *manager,
                                              const char *id);
bool workflow_manager_set_enabled(workflow_manager_t *manager,
                                   const char *id, bool enabled);
bool workflow_manager_set_selected(workflow_manager_t *manager, const char *id);
workflow_t *workflow_manager_selected(workflow_manager_t *manager);
const workflow_t *workflow_manager_selected_const(const workflow_manager_t *manager);

#ifdef __cplusplus
}
#endif
