#pragma once

#include "workflow-model.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void workflow_node_identity_generate(char *id, size_t size);
bool workflow_manager_generate_node_id(const workflow_manager_t *manager,
                                        char *id, size_t size);
bool workflow_manager_node_ids_unique(const workflow_manager_t *manager);
void workflow_manager_repair_node_ids(workflow_manager_t *manager);
bool workflow_node_belongs_to_workflow(const workflow_t *workflow, const char *node_id);

#ifdef __cplusplus
}
#endif
