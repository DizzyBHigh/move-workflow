#include "workflow-scene.h"
#include "workflow-connection-editor.h"
#include "workflow-editor-connections.hpp"
#include "workflow-node-identity.hpp"
#include "workflow-scene-utils.h"

#include <obs.h>
#include <QMessageBox>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>

#include <cstring>
#include <utility>

namespace {
struct ConnectionEditContext { EditorScene *scene = nullptr; QGraphicsPathItem *line = nullptr; };
static void handle_connection_edit(void *context, const QString &type)
{
    auto *edit = static_cast<ConnectionEditContext *>(context);
    if (edit && edit->scene)
        edit->scene->editConnection(edit->line, type);
}
}

QGraphicsPathItem *EditorScene::connectionAt(const QPointF &scenePos) const
{
    QPainterPathStroker stroker;
    stroker.setWidth(12.0);
    for (auto it = connections_.crbegin(); it != connections_.crend(); ++it) {
        if (!it->line) continue;
        const QPointF local = it->line->mapFromScene(scenePos);
        if (stroker.createStroke(it->line->path()).contains(local)) return it->line;
    }
    return nullptr;
}

EditorScene::Connection *EditorScene::findConnection(QGraphicsPathItem *line)
{
    if (!line) return nullptr;
    for (Connection &connection : connections_)
        if (connection.line == line) return &connection;
    return nullptr;
}

bool EditorScene::editConnection(QGraphicsPathItem *line, const QString &type)
{
    Connection *connection = findConnection(line);
    if (!connection) return false;
    auto *wf = connection->from->workflowNode();
    const QString target = workflow_editor_connections::target_id(connection->to);
    workflow_scene_utils::remove_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, target);
    workflow_scene_utils::remove_id(wf->next_node_count, wf->next_node_ids, target);
    workflow_scene_utils::remove_id(wf->shortcut_node_count, wf->shortcut_node_ids, target);
    if (type == "__delete__") { rebuildConnections(); emit workflowChanged(); return true; }
    if (type == "Simultaneous") workflow_scene_utils::add_node_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, target);
    else if (type == "Next") workflow_scene_utils::add_node_id(wf->next_node_count, wf->next_node_ids, target);
    else if (type == "Shortcut") workflow_scene_utils::add_node_id(wf->shortcut_node_count, wf->shortcut_node_ids, target);
    else return false;
    connection->type = type == "Next" ? "Next Action" : type;
    connection->from->refreshDisplay(); connection->to->refreshDisplay();
    rebuildConnections(); emit workflowChanged(); return true;
}

NodeItem *EditorScene::nodeAt(const QPointF &scenePos) const
{
    return workflow_editor_connections::node_at(const_cast<EditorScene *>(this), scenePos);
}

NodeItem *EditorScene::findNodeById(const workflow_t *workflow, const char *id) const
{
    if (!workflow || !id || !workflow_node_belongs_to_workflow(workflow, id)) return nullptr;
    for (NodeItem *node : nodes_)
        if (node && node->workflowNode() && node->workflowNode()->id[0] &&
            strcmp(node->workflowNode()->id, id) == 0) return node;
    return nullptr;
}

void EditorScene::finishConnectionDrag(const QPointF &scenePos)
{
    NodeItem *source = dragSource_;
    NodeItem *target = workflow_editor_connections::node_at(this, scenePos);
    if (dragPreview_) { removeItem(dragPreview_); delete dragPreview_; }
    dragPreview_ = nullptr; dragSource_ = nullptr; draggingConnection_ = false;
    if (!source || !target || source == target ||
        source->workflowNode() != target->workflowNode()) return;

    blog(LOG_DEBUG, "[Move Workflow] Connection target: '%s' -> '%s' (%s)",
         source->id().toUtf8().constData(), target->id().toUtf8().constData(),
         target->nodeName().toUtf8().constData());

    const auto sourceType = source->workflowNode()->type;
    const auto targetType = target->workflowNode()->type;
    if (sourceType == WORKFLOW_NODE_ACTION && targetType == WORKFLOW_NODE_ACTION) {
        QMessageBox dialog(QMessageBox::Question, "Connect Actions", "Choose how the destination Action should start.", QMessageBox::NoButton, nullptr);
        auto *simultaneous = dialog.addButton("Simultaneous", QMessageBox::AcceptRole);
        auto *next = dialog.addButton("Next", QMessageBox::AcceptRole);
        auto *shortcut = dialog.addButton("Shortcut", QMessageBox::AcceptRole);
        auto *cancel = dialog.addButton("Cancel", QMessageBox::RejectRole);
        dialog.exec();
        if (dialog.clickedButton() == simultaneous) connectActionToAction(source, target, "Simultaneous");
        else if (dialog.clickedButton() == next) connectActionToAction(source, target, "Next");
        else if (dialog.clickedButton() == shortcut) connectActionToAction(source, target, "Shortcut");
        else return;
    } else if (sourceType == WORKFLOW_NODE_TRIGGER && targetType == WORKFLOW_NODE_ACTION) {
        connectTriggerToAction(source, target);
    } else if (sourceType == WORKFLOW_NODE_ACTION && targetType == WORKFLOW_NODE_TRIGGER) {
        connectTriggerToAction(target, source);
    } else return;
    source->refreshDisplay(); target->refreshDisplay(); rebuildConnections(); emit workflowChanged();
}

void EditorScene::connectTriggerToAction(NodeItem *trigger, NodeItem *action)
{
    if (!trigger || !action) return;
    workflow_scene_utils::add_node_id(trigger->workflowNode()->simultaneous_node_count,
                                      trigger->workflowNode()->simultaneous_node_ids, action->id());
}

void EditorScene::connectActionToAction(NodeItem *source, NodeItem *target, const QString &type)
{
    auto *wf = source->workflowNode();
    const QString id = target->id();
    workflow_scene_utils::remove_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, id);
    workflow_scene_utils::remove_id(wf->next_node_count, wf->next_node_ids, id);
    workflow_scene_utils::remove_id(wf->shortcut_node_count, wf->shortcut_node_ids, id);
    if (type == "Simultaneous") workflow_scene_utils::add_node_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, id);
    else if (type == "Next") workflow_scene_utils::add_node_id(wf->next_node_count, wf->next_node_ids, id);
    else workflow_scene_utils::add_node_id(wf->shortcut_node_count, wf->shortcut_node_ids, id);
}

void EditorScene::addRelationshipLines(NodeItem *from, size_t count, const char ids[][WORKFLOW_MAX_NAME], const QString &type)
{
    if (!from || !from->workflowNode()) return;
    const workflow_t *workflow = from->workflowNode()->workflow;
    for (size_t i = 0; i < count; ++i) {
        NodeItem *to = findNodeById(workflow, ids[i]);
        if (!to || to == from) continue;
        auto *line = new QGraphicsPathItem;
        if (type == "Simultaneous") line->setPen(QPen(QColor(90, 190, 120), 2));
        else if (type == "Next Action") line->setPen(QPen(QColor(230, 170, 70), 2));
        else line->setPen(QPen(QColor(180, 120, 220), 2));
        line->setBrush(line->pen().color());
        line->setZValue(-1); addItem(line); connections_.push_back({from, to, line, type});
    }
}

void EditorScene::updateConnection(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
{
    workflow_editor_connections::update_path(line, from, to);
}
