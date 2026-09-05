#include "workflow-scene.h"
#include "workflow-scene-relationship.h"
#include "workflow-editor-connections.hpp"

#include <obs.h>
#include <QMessageBox>
#include <QPushButton>

bool EditorScene::editConnection(QGraphicsPathItem *line, const QString &type)
{
    Connection *connection = findConnection(line);
    if (!connection)
        return false;

    auto *source = connection->from->workflowNode();
    auto *target = connection->to->workflowNode();
    if (!source || !target)
        return false;

    if (type == "__delete__") {
        const QString current = workflow_scene_relationship::type_between(source, target);
        if (current.isEmpty())
            return false;
        workflow_scene_relationship::remove(source, target, current);
        rebuildConnections();
        emit workflowChanged();
        return true;
    }

    workflow_scene_relationship::remove(source, target, "Simultaneous");
    workflow_scene_relationship::remove(source, target, "Next");
    workflow_scene_relationship::remove(source, target, "Shortcut");
    if (!workflow_scene_relationship::add(source, target, type))
        return false;

    connection->type = type == "Next" ? "Next Action" : type;
    connection->from->refreshDisplay();
    connection->to->refreshDisplay();
    rebuildConnections();
    emit workflowChanged();
    return true;
}

bool EditorScene::deleteMissingConnection(NodeItem *from,
                                           const QString &targetId,
                                           const QString &type)
{
    if (!from || targetId.isEmpty())
        return false;
    auto *source = from->workflowNode();
    if (!source)
        return false;

    for (NodeItem *node : nodes_) {
        if (!node || node->id() != targetId)
            continue;
        auto *target = node->workflowNode();
        if (!target)
            return false;
        if (!workflow_scene_relationship::remove(source, target, type))
            return false;
        from->refreshDisplay();
        rebuildConnections();
        emit workflowChanged();
        return true;
    }
    return false;
}

void EditorScene::finishConnectionDrag(const QPointF &scenePos)
{
    NodeItem *source = dragSource_;
    NodeItem *target = workflow_editor_connections::node_at(this, scenePos);
    if (dragPreview_) {
        removeItem(dragPreview_);
        delete dragPreview_;
    }
    dragPreview_ = nullptr;
    dragSource_ = nullptr;
    draggingConnection_ = false;
    if (!source || !target || source == target)
        return;

    const auto sourceType = source->workflowNode()->type;
    const auto targetType = target->workflowNode()->type;
    if (sourceType == WORKFLOW_NODE_ACTION && targetType == WORKFLOW_NODE_ACTION) {
        QMessageBox dialog(QMessageBox::Question, "Connect Actions",
                           "Choose how the destination Action should start.",
                           QMessageBox::NoButton, nullptr);
        auto *simultaneous = dialog.addButton("Simultaneous", QMessageBox::AcceptRole);
        auto *next = dialog.addButton("Next", QMessageBox::AcceptRole);
        auto *shortcut = dialog.addButton("Shortcut", QMessageBox::AcceptRole);
        auto *cancel = dialog.addButton("Cancel", QMessageBox::RejectRole);
        dialog.exec();
        if (dialog.clickedButton() == simultaneous)
            connectActionToAction(source, target, "Simultaneous");
        else if (dialog.clickedButton() == next)
            connectActionToAction(source, target, "Next");
        else if (dialog.clickedButton() == shortcut)
            connectActionToAction(source, target, "Shortcut");
        else if (dialog.clickedButton() == cancel)
            return;
        else
            return;
    } else if (sourceType == WORKFLOW_NODE_TRIGGER && targetType == WORKFLOW_NODE_ACTION) {
        connectTriggerToAction(source, target);
    } else if (sourceType == WORKFLOW_NODE_ACTION && targetType == WORKFLOW_NODE_TRIGGER) {
        connectTriggerToAction(target, source);
    } else {
        return;
    }
    source->refreshDisplay();
    target->refreshDisplay();
    rebuildConnections();
    emit workflowChanged();
}

void EditorScene::connectTriggerToAction(NodeItem *trigger, NodeItem *action)
{
    if (!trigger || !action)
        return;
    workflow_scene_relationship::add(trigger->workflowNode(), action->workflowNode(), "Simultaneous");
}

void EditorScene::connectActionToAction(NodeItem *source, NodeItem *target,
                                         const QString &type)
{
    if (!source || !target)
        return;
    auto *wf = source->workflowNode();
    workflow_scene_relationship::remove(wf, target->workflowNode(), "Simultaneous");
    workflow_scene_relationship::remove(wf, target->workflowNode(), "Next");
    workflow_scene_relationship::remove(wf, target->workflowNode(), "Shortcut");
    workflow_scene_relationship::add(wf, target->workflowNode(), type);
    blog(LOG_INFO, "[Move Workflow] CONNECT: source=%s target=%s type=%s",
         source->id().toUtf8().constData(), target->id().toUtf8().constData(),
         type.toUtf8().constData());
}
