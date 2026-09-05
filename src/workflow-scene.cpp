#include "workflow-scene.h"
#include "workflow-scene-utils.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QPen>
#include <QUuid>

#include <utility>

EditorScene::EditorScene(QObject *parent) : QGraphicsScene(parent) {}

void EditorScene::setWorkflowId(const QString &workflowId)
{
    workflowId_ = workflowId;
    for (NodeItem *node : nodes_)
        if (node) node->setWorkflowId(workflowId_);
}

NodeItem *EditorScene::addNode(workflow_node_type_t type, const QString &name)
{
    EditorNode node;
    workflow_scene_utils::copy_text(node.workflow.id, WORKFLOW_MAX_NAME,
                                    QUuid::createUuid().toString(QUuid::WithoutBraces));
    workflow_scene_utils::copy_text(node.workflow.name, WORKFLOW_MAX_NAME, name);
    node.workflow.type = type; node.workflow.trigger_count = 0;
    node.workflow.duration.mode = WORKFLOW_OVERRIDE;
    node.workflow.start_delay.mode = WORKFLOW_OVERRIDE;
    node.workflow.end_delay.mode = WORKFLOW_OVERRIDE;
    node.workflow.simultaneous_actions_mode = WORKFLOW_OVERRIDE;
    node.workflow.next_actions_mode = WORKFLOW_OVERRIDE;
    auto *item = new NodeItem(node);
    item->setWorkflowId(workflowId_);
    item->setWorkflowChangedCallback([this] { emit workflowChanged(); });
    addItem(item); nodes_.push_back(item); updateSceneBounds();
    emit workflowChanged();
    return item;
}

NodeItem *EditorScene::selectedNode() const
{
    for (QGraphicsItem *item : selectedItems()) if (auto *node = dynamic_cast<NodeItem *>(item)) return node;
    return nullptr;
}

QList<NodeItem *> EditorScene::nodes() const { return nodes_; }

void EditorScene::deleteNode(NodeItem *node)
{
    if (!node) return; const QString id = node->id();
    for (NodeItem *candidate : nodes_) {
        auto *wf = candidate->workflowNode();
        workflow_scene_utils::remove_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, id);
        workflow_scene_utils::remove_id(wf->next_node_count, wf->next_node_ids, id);
        workflow_scene_utils::remove_id(wf->shortcut_node_count, wf->shortcut_node_ids, id);
    }
    nodes_.removeAll(node); removeItem(node); delete node; rebuildConnections(); updateSceneBounds();
    emit workflowChanged();
}

void EditorScene::refreshConnectionsFor(NodeItem *) { rebuildConnections(); updateSceneBounds(); }

void EditorScene::rebuildConnections()
{
    for (const Connection &connection : std::as_const(connections_)) { removeItem(connection.line); delete connection.line; }
    for (const MissingConnection &connection : std::as_const(missingConnections_)) { removeItem(connection.line); delete connection.line; }
    missingConnections_.clear();
    connections_.clear();
    if (missingNode_) { removeItem(missingNode_); delete missingNode_; missingNode_ = nullptr; }
    missingNodeLabel_ = nullptr;
    for (NodeItem *from : nodes_) {
        const auto *wf = from->workflowNode();
        addRelationshipLines(from, wf->simultaneous_node_count, wf->simultaneous_node_ids, "Simultaneous");
        addRelationshipLines(from, wf->next_node_count, wf->next_node_ids, "Next Action");
        addRelationshipLines(from, wf->shortcut_node_count, wf->shortcut_node_ids, "Shortcut");
    }
    rebuildMissingNode();
    updateConnections();
}

void EditorScene::rebuildMissingNode()
{
    if (missingConnections_.isEmpty()) return;
    missingNode_ = new QGraphicsRectItem(QRectF(0, 0, 230, 110));
    missingNode_->setBrush(QColor(55, 25, 25));
    missingNode_->setPen(QPen(QColor(230, 70, 70), 2, Qt::DashLine));
    missingNode_->setZValue(-0.5); addItem(missingNode_);
    QStringList ids;
    for (const MissingConnection &connection : std::as_const(missingConnections_))
        if (!ids.contains(connection.targetId)) ids.append(connection.targetId);
    QString text = QString("TRASH / MISSING NODES\n%1 missing target%2")
                       .arg(ids.size()).arg(ids.size() == 1 ? "" : "s");
    if (ids.size() <= 4) text += "\n" + ids.join(", ");
    missingNodeLabel_ = new QGraphicsTextItem(text, missingNode_);
    missingNodeLabel_->setDefaultTextColor(QColor(255, 170, 170));
    missingNodeLabel_->setPos(12, 10);
    QRectF bounds;
    bool valid = false;
    for (NodeItem *node : nodes_) {
        const QRectF rect = node->sceneBoundingRect();
        bounds = valid ? bounds.united(rect) : rect; valid = true;
    }
    const QPointF pos = valid ? QPointF(bounds.right() + 180, bounds.center().y() - 55) : QPointF(200, 200);
    missingNode_->setPos(pos);
}

void EditorScene::updateConnections(bool updateBounds)
{
    for (const Connection &connection : std::as_const(connections_)) updateConnection(connection.line, connection.from, connection.to);
    for (const MissingConnection &connection : std::as_const(missingConnections_))
        if (connection.line && connection.from && missingNode_) {
            const QRectF source = connection.from->sceneBoundingRect();
            const QRectF target = missingNode_->sceneBoundingRect();
            const QPointF start(source.right(), source.center().y());
            const QPointF end(target.left(), target.center().y());
            const qreal dx = end.x() - start.x();
            QPainterPath path(start);
            path.cubicTo(start + QPointF(dx * 0.35, 0), end - QPointF(dx * 0.35, 0), end);
            connection.line->setPath(path);
        }
    if (draggingConnection_ && dragPreview_ && dragSource_) {
        const QPointF start = dragPreview_->path().elementAt(0), end = dragPreview_->path().currentPosition();
        const qreal dx=end.x()-start.x(), dy=end.y()-start.y(); QPainterPath path(start);
        path.cubicTo(start+QPointF(dx*0.35,dy*0.05),end-QPointF(dx*0.35,dy*0.05),end); dragPreview_->setPath(path);
    }
    if (updateBounds) updateSceneBounds();
}

void EditorScene::updateSceneBounds()
{
    if (nodes_.isEmpty()) { setSceneRect(0,0,2000,1400); return; }
    QRectF bounds; bool valid=false;
    for (NodeItem *node : nodes_) { const QRectF rect=node->sceneBoundingRect(); bounds=valid?bounds.united(rect):rect; valid=true; }
    if (missingNode_) bounds = bounds.united(missingNode_->sceneBoundingRect());
    constexpr qreal margin=160.0, minimumWidth=900.0, minimumHeight=600.0; bounds.adjust(-margin,-margin,margin,margin);
    if (bounds.width()<minimumWidth) { const qreal extra=(minimumWidth-bounds.width())*0.5; bounds.adjust(-extra,0,extra,0); }
    if (bounds.height()<minimumHeight) { const qreal extra=(minimumHeight-bounds.height())*0.5; bounds.adjust(0,-extra,0,extra); }
    setSceneRect(bounds);
}