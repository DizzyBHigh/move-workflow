#pragma once

#include "workflow-manager.h"

class EditorScene;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct workflow_workspace {
    workflow_manager_t manager;
    EditorScene *scene;
    char loaded_workflow_id[WORKFLOW_MAX_NAME];
} workflow_workspace_t;

void workflow_workspace_init(workflow_workspace_t *workspace, EditorScene *scene);
void workflow_workspace_sync_scene(workflow_workspace_t *workspace);
bool workflow_workspace_select(workflow_workspace_t *workspace, const char *id);
bool workflow_workspace_create(workflow_workspace_t *workspace, const char *name);
bool workflow_workspace_duplicate(workflow_workspace_t *workspace, const char *name);
workflow_manager_t *workflow_workspace_manager(workflow_workspace_t *workspace);

#ifdef __cplusplus
}
#endif
