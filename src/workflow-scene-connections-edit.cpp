#include "workflow-scene.h"
#include "workflow-editor-connections.hpp"
#include "workflow-scene-relationship.h"

#include <obs.h>
#include <QGraphicsPathItem>
#include <QMessageBox>
#include <QPushButton>

namespace {
static bool valid_connection_nodes(NodeItem *source, NodeItem *target)
{
    return source && target && source != target && !source->id().isEmpty() &&
           !target->id().isEmpty();
}
}

bool EditorScene::editConnection(QGraphicsPathItem *line, const QString &type)
{
    Connection *connection = findConnection(line);
    if (!connection) return false;
    auto *source = connection->from->workflowNode();
    auto *target = connection->to->workflowNode();
    if (!source || !target) return false;

    const QString oldType = connection->type;
    const QString targetId = connection->to->id();
    if (type == oldType) return true;
    if (!workflow_scene_relationship::remove(source, target, oldType)) return false;
    if (type == "__delete__") {
        rebuildConnections();
        emit workflowChanged();
        return true;
    }
    if (!workflow_scene_relationship::add(source, target, type)) return false;
    connection->type = type == "Next" ? "Next Action" : type;
    connection->from->refreshDisplay();
    connection->to->refreshDisplay();
    rebuildConnections();
    emit workflowChanged();
    blog(LOG_DEBUG, "[Move Workflow] EDIT: target=%s type=%s", targetId.toUtf8().constData(),
         type.toUtf8().constData());
    return true;
}

bool EditorScene::deleteMissingConnection(NodeItem *from, const QString &targetId,
                                          const QString &type)
{
    if (!from || targetId.isEmpty()) return false;
    NodeItem *target = findNodeById(targetId.toUtf8().constData());
    if (!target) return false;
    auto *wf = from->workflowNode();
    auto *targetWf = target->workflowNode();
    if (!wf || !targetWf) return false;
    if (!workflow_scene_relationship::remove(wf, targetWf, type)) return false;
    from->refreshDisplay();
    rebuildConnections();
    emit workflowChanged();
    return true;
}

void EditorScene::finishConnectionDrag(const QPointF &scenePos)
{
    NodeItem *source = dragSource_;
    NodeItem *target = workflow_editor_connections::node_at(this, scenePos);
    if (dragPreview_) { removeItem(dragPreview_); delete dragPreview_; }
    dragPreview_ = nullptr;
    dragSource_ = nullptr;
    draggingConnection_ = false;
    if (!valid_connection_nodes(source, target)) return;

    const auto sourceType = source->workflowNode()->type;
    const auto targetType = target->workflowNode()->type;
    if (sourceType == WORKFLOW_NODE_ACTION && targetType == WORKFLOW_NODE_ACTION) {
        QMessageBox dialog(QMessageBox::Question, "Connect Actions",
                           "Choose how the destination Action should start.",
                           QMessageBox::NoButton, nullptr);
        auto *simultaneous = dialog.addButton("Simultaneous", QMessageBox::AcceptRole);
        auto *next = dialog.addButton("Next", QMessageBox::AcceptRole);
        auto *shortcut = dialog.addButton("Shortcut", QMessageBox::AcceptRole);
        dialog.addButton("Cancel", QMessageBox::RejectRole);
        dialog.exec();
        if (dialog.clickedButton() == simultaneous)
            connectActionToAction(source, target, "Simultaneous");
        else if (dialog.clickedButton() == next)
            connectActionToAction(source, target, "Next");
        else if (dialog.clickedButton() == shortcut)
            connectActionToAction(source, target, "Shortcut");
        else return;
    } else if (sourceType == WORKFLOW_NODE_TRIGGER && targetType == WORKFLOW_NODE_ACTION) {
        connectTriggerToAction(source, target);
    } else if (sourceType == WORKFLOW_NODE_ACTION && targetType == WORKFLOW_NODE_TRIGGER) {
        connectTriggerToAction(target, source);
    } else return;
    source->refreshDisplay();
    target->refreshDisplay();
    rebuildConnections();
    emit workflowChanged();
}

void EditorScene::connectTriggerToAction(NodeItem *trigger, NodeItem *action)
{
    if (!valid_connection_nodes(trigger, action)) return;
    workflow_scene_relationship::add(trigger->workflowNode(), action->workflowNode(),
                                     "Simultaneous");
}

void EditorScene::connectActionToAction(NodeItem *source, NodeItem *target,
                                        const QString &type)
{
    if (!valid_connection_nodes(source, target)) return;
    auto *wf = source->workflowNode();
    auto *targetWf = target->workflowNode();
    if (!wf || !targetWf) return;
    const QString id = target->id();
    const QString oldType = workflow_scene_relationship::type_between(wf, targetWf);
    if (!oldType.isEmpty() && oldType != type)
        workflow_scene_relationship::remove(wf, targetWf, oldType);
    if (!workflow_scene_relationship::add(wf, targetWf, type)) return;
    blog(LOG_INFO, "[Move Workflow] CONNECT: source=%s target=%s type=%s",
         source->id().toUtf8().constData(), id.toUtf8().constData(),
         type.toUtf8().constData());
}
