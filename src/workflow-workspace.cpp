#include "workflow-workspace.h"

#include "workflow-scene.h"

#include <QList>
#include <cstring>

namespace {
void copy_scene_to_workflow(EditorScene *scene, workflow_t *workflow)
{
    if (!scene || !workflow)
        return;
    const QList<NodeItem *> nodes = scene->nodes();
    workflow->node_count = 0;
    workflow->entry_node_count = 0;
    for (NodeItem *item : nodes) {
        if (!item || workflow->node_count >= WORKFLOW_MAX_NODES)
            continue;
        workflow->nodes[workflow->node_count++] = *item->workflowNode();
    }
}

void clear_scene(EditorScene *scene)
{
    if (!scene)
        return;
    const QList<NodeItem *> nodes = scene->nodes();
    for (NodeItem *item : nodes)
        scene->deleteNode(item);
}

void load_workflow(EditorScene *scene, const workflow_t *workflow)
{
    if (!scene || !workflow)
        return;
    clear_scene(scene);
    for (size_t i = 0; i < workflow->node_count; ++i) {
        const workflow_node_t &source = workflow->nodes[i];
        NodeItem *item = scene->addNode(source.type, QString::fromUtf8(source.name));
        if (item)
            *item->workflowNode() = source;
    }
    scene->rebuildConnections();
    scene->updateSceneBounds();
}
}

void workflow_workspace_init(workflow_workspace_t *workspace, EditorScene *scene)
{
    if (!workspace)
        return;
    workflow_manager_init(&workspace->manager);
    workspace->scene = scene;
    workflow_t *workflow = workflow_manager_create(&workspace->manager, "workflow_1", "New Workflow");
    if (workflow)
        copy_scene_to_workflow(scene, workflow);
}

void workflow_workspace_sync_scene(workflow_workspace_t *workspace)
{
    if (!workspace || !workspace->scene)
        return;
    workflow_t *workflow = workflow_manager_selected(&workspace->manager);
    copy_scene_to_workflow(workspace->scene, workflow);
}

bool workflow_workspace_select(workflow_workspace_t *workspace, const char *id)
{
    if (!workspace || !id)
        return false;
    workflow_workspace_sync_scene(workspace);
    if (!workflow_manager_set_selected(&workspace->manager, id))
        return false;
    load_workflow(workspace->scene, workflow_manager_selected_const(&workspace->manager));
    return true;
}

workflow_manager_t *workflow_workspace_manager(workflow_workspace_t *workspace)
{
    return workspace ? &workspace->manager : nullptr;
}
