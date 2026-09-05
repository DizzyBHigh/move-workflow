#include "workflow-workspace.h"
#include "workflow-manager-copy.h"
#include "workflow-node-identity.hpp"
#include "workflow-persistence.h"
#include "workflow-scene.h"
#include <QList>
#include <cstdio>
#include <cstring>

namespace {
void copy_scene_to_workflow(EditorScene *scene, workflow_t *workflow)
{
    if (!scene || !workflow) return;
    const QList<NodeItem *> nodes = scene->nodes();
    const size_t old_entry_count = workflow->entry_node_count;
    char old_entries[WORKFLOW_MAX_NODES][WORKFLOW_MAX_NAME] = {};
    for (size_t i = 0; i < old_entry_count && i < WORKFLOW_MAX_NODES; ++i)
        std::snprintf(old_entries[i], WORKFLOW_MAX_NAME, "%s", workflow->entry_node_ids[i]);

    workflow->node_count = 0;
    for (NodeItem *item : nodes) {
        if (!item || workflow->node_count >= WORKFLOW_MAX_NODES) continue;
        workflow_node_t node = *item->workflowNode();
        const QPointF pos = item->pos();
        node.position_x = qRound(pos.x());
        node.position_y = qRound(pos.y());
        workflow->nodes[workflow->node_count++] = node;
    }

    workflow->entry_node_count = 0;
    for (size_t i = 0; i < old_entry_count && i < WORKFLOW_MAX_NODES; ++i) {
        if (!old_entries[i][0]) continue;
        if (!workflow_node_belongs_to_workflow(workflow, old_entries[i])) continue;
        std::snprintf(workflow->entry_node_ids[workflow->entry_node_count++],
                      WORKFLOW_MAX_NAME, "%s", old_entries[i]);
    }
}
void clear_scene(EditorScene *scene)
{
    if (!scene) return; const QList<NodeItem *> nodes=scene->nodes();
    for (NodeItem *item : nodes) scene->deleteNode(item);
}
void load_workflow(EditorScene *scene, const workflow_t *workflow)
{
    if (!scene || !workflow) return;
    scene->setWorkflowId(QString::fromUtf8(workflow->id));
    const bool blocked=scene->blockSignals(true); clear_scene(scene);
    for (size_t i=0;i<workflow->node_count;++i) {
        const workflow_node_t &source=workflow->nodes[i]; NodeItem *item=scene->addNode(source.type,QString::fromUtf8(source.name));
        if (!item) continue; *item->workflowNode()=source;
        if (!item->workflowNode()->id[0]) workflow_node_identity_generate(item->workflowNode()->id,sizeof(item->workflowNode()->id));
        item->setPos(source.position_x,source.position_y); item->refreshDisplay();
    }
    scene->rebuildConnections(); scene->updateSceneBounds(); scene->blockSignals(blocked);
}
bool make_id(const workflow_manager_t *manager,const char *prefix,char *id,size_t capacity)
{
    if (!manager||!prefix||!id||capacity==0) return false;
    for(size_t n=1;n<=WORKFLOW_MANAGER_MAX_WORKFLOWS;++n){std::snprintf(id,capacity,"%s_%zu",prefix,n);if(!workflow_manager_find_const(manager,id))return true;} return false;
}
void set_loaded_id(workflow_workspace_t *workspace,const char *id)
{
    if (!workspace) return; std::strncpy(workspace->loaded_workflow_id,id?id:"",WORKFLOW_MAX_NAME-1);
    workspace->loaded_workflow_id[WORKFLOW_MAX_NAME-1]='\0';
}
void ensure_scene_node_ids(workflow_workspace_t *workspace)
{
    if (!workspace||!workspace->scene)return; QList<NodeItem *> nodes=workspace->scene->nodes();
    for(NodeItem *item:nodes){if(!item)continue;workflow_node_t *node=item->workflowNode();if(!node->id[0])workflow_manager_generate_node_id(&workspace->manager,node->id,sizeof(node->id));}
}
}

void workflow_workspace_init(workflow_workspace_t *workspace,EditorScene *scene)
{
    if(!workspace)return; workflow_persistence_init(); const workflow_manager_t *saved=workflow_persistence_manager();
    workflow_manager_init(&workspace->manager); if(saved&&saved->workflow_count)workspace->manager=*saved;
    workflow_manager_repair_node_ids(&workspace->manager); workspace->scene=scene; workspace->loaded_workflow_id[0]='\0';
    if(!workspace->manager.workflow_count){workflow_t *workflow=workflow_manager_create(&workspace->manager,"workflow_1","New Workflow");if(!workflow)return;workflow_manager_set_selected(&workspace->manager,workflow->id);}
    workflow_t *workflow=workflow_manager_selected(&workspace->manager);
    if(workflow){load_workflow(scene,workflow);set_loaded_id(workspace,workflow->id);} workflow_persistence_sync(&workspace->manager);
}

void workflow_workspace_sync_scene(workflow_workspace_t *workspace)
{
    if(!workspace||!workspace->scene||!workspace->loaded_workflow_id[0])return;
    ensure_scene_node_ids(workspace);
    workflow_t *workflow=workflow_manager_find(&workspace->manager,workspace->loaded_workflow_id);
    if(!workflow)return;
    const workflow_t before=*workflow;
    copy_scene_to_workflow(workspace->scene,workflow);
    workflow_manager_repair_node_ids(&workspace->manager);
    const bool changed=std::memcmp(&before,workflow,sizeof(workflow_t))!=0;
    if(changed) workflow_persistence_sync(&workspace->manager);
}
void workflow_workspace_reload(workflow_workspace_t *workspace)
{if(!workspace||!workspace->loaded_workflow_id[0])return;const workflow_t *workflow=workflow_manager_find_const(&workspace->manager,workspace->loaded_workflow_id);load_workflow(workspace->scene,workflow);}
bool workflow_workspace_select(workflow_workspace_t *workspace,const char *id)
{if(!workspace||!id||!workflow_manager_find_const(&workspace->manager,id))return false;workflow_workspace_sync_scene(workspace);if(!workflow_manager_set_selected(&workspace->manager,id))return false;load_workflow(workspace->scene,workflow_manager_selected_const(&workspace->manager));set_loaded_id(workspace,id);workflow_persistence_sync(&workspace->manager);return true;}
bool workflow_workspace_create(workflow_workspace_t *workspace,const char *name)
{if(!workspace)return false;workflow_workspace_sync_scene(workspace);char id[WORKFLOW_MAX_NAME]={};if(!make_id(&workspace->manager,"workflow",id,sizeof(id)))return false;workflow_t *workflow=workflow_manager_create(&workspace->manager,id,name);if(!workflow)return false;workflow_manager_set_selected(&workspace->manager,workflow->id);load_workflow(workspace->scene,workflow);set_loaded_id(workspace,workflow->id);workflow_persistence_sync(&workspace->manager);return true;}
bool workflow_workspace_duplicate(workflow_workspace_t *workspace,const char *name)
{if(!workspace)return false;workflow_workspace_sync_scene(workspace);const workflow_t *source=workflow_manager_find_const(&workspace->manager,workspace->loaded_workflow_id);if(!source)return false;char id[WORKFLOW_MAX_NAME]={};if(!make_id(&workspace->manager,"workflow_copy",id,sizeof(id)))return false;workflow_t *copy=workflow_manager_duplicate(&workspace->manager,source->id,id,name);if(!copy)return false;load_workflow(workspace->scene,copy);set_loaded_id(workspace,copy->id);workflow_persistence_sync(&workspace->manager);return true;}
workflow_manager_t *workflow_workspace_manager(workflow_workspace_t *workspace){return workspace?&workspace->manager:nullptr;}
const workflow_manager_t *workflow_workspace_manager_const(const workflow_workspace_t *workspace){return workspace?&workspace->manager:nullptr;}