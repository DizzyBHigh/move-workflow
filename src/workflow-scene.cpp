#include "workflow-scene.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QTransform>

#include <cmath>
#include <cstring>
#include <utility>

namespace {

static void copy_text(char *destination, size_t capacity, const QString &value)
{
    if (!destination || capacity == 0)
        return;
    const QByteArray bytes = value.toUtf8();
    std::strncpy(destination, bytes.constData(), capacity - 1);
    destination[capacity - 1] = '\0';
}

static void remove_id(size_t &count, char ids[][WORKFLOW_MAX_NAME], const QString &id)
{
    const QByteArray wanted = id.toUtf8();
    size_t write = 0;
    for (size_t read = 0; read < count; ++read) {
        if (std::strcmp(ids[read], wanted.constData()) != 0) {
            if (write != read)
                std::memcpy(ids[write], ids[read], WORKFLOW_MAX_NAME);
            ++write;
        }
    }
    count = write;
}

} // namespace

EditorScene::EditorScene(QObject *parent) : QGraphicsScene(parent)
{
    connect(this, &QGraphicsScene::changed, this, [this] { updateConnections(); });
}

NodeItem *EditorScene::addNode(workflow_node_type_t type, const QString &name)
{
    EditorNode node;
    node.numeric_id = ++nextId_;
    copy_text(node.workflow.id, WORKFLOW_MAX_NAME, QString("node-%1").arg(node.numeric_id));
    copy_text(node.workflow.name, WORKFLOW_MAX_NAME, name);
    node.workflow.type = type;
    node.workflow.trigger.type = WORKFLOW_TRIGGER_NONE;
    node.workflow.trigger.state = WORKFLOW_TRIGGER_STATE_ENABLED;
    copy_text(node.workflow.trigger.action, WORKFLOW_MAX_NAME, "None");
    node.workflow.duration.mode = WORKFLOW_OVERRIDE;
    node.workflow.start_delay.mode = WORKFLOW_OVERRIDE;
    node.workflow.end_delay.mode = WORKFLOW_OVERRIDE;
    node.workflow.simultaneous_actions_mode = WORKFLOW_OVERRIDE;
    node.workflow.end_actions_mode = WORKFLOW_OVERRIDE;
    node.workflow.next_actions_mode = WORKFLOW_OVERRIDE;
    node.position = QPointF(80 + ((node.numeric_id - 1) % 4) * 310,
                            80 + ((node.numeric_id - 1) / 4) * 190);

    auto *item = new NodeItem(node);
    addItem(item);
    nodes_.push_back(item);
    updateSceneBounds();
    return item;
}

NodeItem *EditorScene::selectedNode() const
{
    for (QGraphicsItem *item : selectedItems()) {
        if (auto *node = dynamic_cast<NodeItem *>(item))
            return node;
    }
    return nullptr;
}

QList<NodeItem *> EditorScene::nodes() const
{
    return nodes_;
}

void EditorScene::deleteNode(NodeItem *node)
{
    if (!node)
        return;
    const QString id = node->id();
    for (NodeItem *candidate : nodes_) {
        workflow_node_t *wf = candidate->workflowNode();
        remove_id(wf->end_node_count, wf->end_node_ids, id);
        remove_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, id);
        remove_id(wf->next_node_count, wf->next_node_ids, id);
    }
    nodes_.removeAll(node);
    removeItem(node);
    delete node;
    rebuildConnections();
    updateSceneBounds();
}

void EditorScene::refreshConnectionsFor(NodeItem *)
{
    rebuildConnections();
    updateSceneBounds();
}

void EditorScene::rebuildConnections()
{
    for (const Connection &connection : std::as_const(connections_)) {
        removeItem(connection.line);
        delete connection.line;
    }
    connections_.clear();

    for (NodeItem *from : nodes_) {
        const workflow_node_t *wf = from->workflowNode();
        addRelationshipLines(from, wf->end_node_count, wf->end_node_ids, "End Action");
        addRelationshipLines(from, wf->simultaneous_node_count, wf->simultaneous_node_ids,
                             "Simultaneous");
        addRelationshipLines(from, wf->next_node_count, wf->next_node_ids, "Next Action");
    }
    updateConnections();
}

void EditorScene::updateConnections()
{
    for (const Connection &connection : std::as_const(connections_))
        updateConnection(connection.line, connection.from, connection.to);
    updateSceneBounds();
}

void EditorScene::updateSceneBounds()
{
    if (nodes_.isEmpty()) {
        setSceneRect(0, 0, 2000, 1400);
        return;
    }
    QRectF bounds;
    bool valid = false;
    for (NodeItem *node : nodes_) {
        const QRectF rect = node->sceneBoundingRect();
        bounds = valid ? bounds.united(rect) : rect;
        valid = true;
    }
    constexpr qreal margin = 160.0;
    constexpr qreal minimumWidth = 900.0;
    constexpr qreal minimumHeight = 600.0;
    bounds.adjust(-margin, -margin, margin, margin);
    if (bounds.width() < minimumWidth) {
        const qreal extra = (minimumWidth - bounds.width()) * 0.5;
        bounds.adjust(-extra, 0, extra, 0);
    }
    if (bounds.height() < minimumHeight) {
        const qreal extra = (minimumHeight - bounds.height()) * 0.5;
        bounds.adjust(0, -extra, 0, extra);
    }
    setSceneRect(bounds);
}

void EditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    while (item && !dynamic_cast<NodeItem *>(item))
        item = item->parentItem();
    if (auto *node = dynamic_cast<NodeItem *>(item))
        emit nodeDoubleClicked(node);
    QGraphicsScene::mouseDoubleClickEvent(event);
}

NodeItem *EditorScene::findNodeById(const char *id) const
{
    for (NodeItem *node : nodes_) {
        if (std::strcmp(node->workflowNode()->id, id) == 0)
            return node;
    }
    return nullptr;
}

void EditorScene::addRelationshipLines(NodeItem *from, size_t count,
                                       const char ids[][WORKFLOW_MAX_NAME],
                                       const QString &type)
{
    for (size_t i = 0; i < count; ++i) {
        NodeItem *to = findNodeById(ids[i]);
        if (!to || to == from)
            continue;
        // Simultaneous/end/next relationships are Action-to-Action links.
        // Never draw a stale link from an Action node back to a Trigger node.
        if (from->workflowNode()->type == WORKFLOW_NODE_ACTION &&
            to->workflowNode()->type == WORKFLOW_NODE_TRIGGER)
            continue;
        auto *line = new QGraphicsPathItem;
        if (type == "Simultaneous")
            line->setPen(QPen(QColor(90, 190, 120), 2));
        else if (type == "Next Action")
            line->setPen(QPen(QColor(230, 170, 70), 2));
        else
            line->setPen(QPen(QColor(70, 160, 230), 2));
        line->setZValue(-1);
        addItem(line);
        connections_.push_back({from, to, line, type});
        updateConnection(line, from, to);
    }
}

void EditorScene::updateConnection(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
{
    const QRectF aRect = from->sceneBoundingRect();
    const QRectF bRect = to->sceneBoundingRect();
    const QPointF fromCenter = aRect.center();
    const QPointF toCenter = bRect.center();
    const qreal dx = toCenter.x() - fromCenter.x();
    const qreal dy = toCenter.y() - fromCenter.y();
    const qreal distance = qMax<qreal>(40.0, std::sqrt(dx * dx + dy * dy) * 0.35);

    QPointF a;
    QPointF b;
    QPointF controlA;
    QPointF controlB;

    if (qAbs(dx) >= qAbs(dy)) {
        if (dx >= 0.0) {
            a = QPointF(aRect.right(), aRect.center().y());
            b = QPointF(bRect.left(), bRect.center().y());
            controlA = a + QPointF(distance, 0);
            controlB = b - QPointF(distance, 0);
        } else {
            a = QPointF(aRect.left(), aRect.center().y());
            b = QPointF(bRect.right(), bRect.center().y());
            controlA = a - QPointF(distance, 0);
            controlB = b + QPointF(distance, 0);
        }
    } else if (dy >= 0.0) {
        a = QPointF(aRect.center().x(), aRect.bottom());
        b = QPointF(bRect.center().x(), bRect.top());
        controlA = a + QPointF(0, distance);
        controlB = b - QPointF(0, distance);
    } else {
        a = QPointF(aRect.center().x(), aRect.top());
        b = QPointF(bRect.center().x(), bRect.bottom());
        controlA = a - QPointF(0, distance);
        controlB = b + QPointF(0, distance);
    }

    QPainterPath path(a);
    path.cubicTo(controlA, controlB, b);
    line->setPath(path);
}
