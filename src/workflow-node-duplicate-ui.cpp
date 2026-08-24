#include "workflow-node-duplicate-ui.h"

#include "workflow-scene.h"

#include <cstring>

NodeItem *duplicate_selected_workflow_node(EditorScene *scene)
{
    if (!scene)
        return nullptr;
    NodeItem *source = scene->selectedNode();
    if (!source)
        return nullptr;

    const workflow_node_t original = *source->workflowNode();
    const QString name = source->nodeName() + " Copy";
    NodeItem *copy = scene->addNode(original.type, name);
    if (!copy)
        return nullptr;

    const QString newId = copy->id();
    *copy->workflowNode() = original;
    const QByteArray idBytes = newId.toUtf8();
    const QByteArray nameBytes = name.toUtf8();
    std::strncpy(copy->workflowNode()->id, idBytes.constData(), WORKFLOW_MAX_NAME - 1);
    copy->workflowNode()->id[WORKFLOW_MAX_NAME - 1] = '\0';
    std::strncpy(copy->workflowNode()->name, nameBytes.constData(), WORKFLOW_MAX_NAME - 1);
    copy->workflowNode()->name[WORKFLOW_MAX_NAME - 1] = '\0';
    copy->workflowNode()->end_node_count = 0;
    copy->workflowNode()->simultaneous_node_count = 0;
    copy->workflowNode()->next_node_count = 0;
    copy->workflowNode()->shortcut_node_count = 0;
    copy->refreshDisplay();
    copy->setSelected(true);
    scene->updateSceneBounds();
    return copy;
}
