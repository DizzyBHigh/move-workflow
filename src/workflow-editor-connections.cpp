#include "workflow-editor-connections.hpp"

#include "workflow-node.h"
#include "workflow-scene.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QLineF>

namespace workflow_editor_connections {

NodeItem *node_at(EditorScene *scene, const QPointF &scene_pos)
{
    if (!scene)
        return nullptr;

    const QList<QGraphicsItem *> items = scene->items(
        scene_pos, Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());
    for (QGraphicsItem *item : items) {
        for (QGraphicsItem *candidate = item; candidate; candidate = candidate->parentItem()) {
            if (auto *node = dynamic_cast<NodeItem *>(candidate))
                return node;
        }
    }
    return nullptr;
}

QString target_id(NodeItem *node)
{
    return node ? node->id() : QString();
}

static QPointF sourcePoint(NodeItem *node)
{
    const QRectF rect = node->sceneBoundingRect();
    return QPointF(rect.center().x(), rect.bottom());
}

static QPointF targetPoint(NodeItem *node)
{
    const QRectF rect = node->sceneBoundingRect();
    return QPointF(rect.center().x(), rect.top());
}

static void addArrow(QPainterPath &path, const QPointF &tip, const QPointF &direction)
{
    const QPointF side(-direction.y(), direction.x());
    const qreal length = 8.0;
    const qreal width = 4.0;
    const QPointF base = tip - direction * length;

    path.moveTo(base + side * width);
    path.lineTo(tip);
    path.lineTo(base - side * width);
}

void update_path(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
{
    if (!line || !from || !to)
        return;

    const QPointF start = sourcePoint(from);
    const QPointF target = targetPoint(to);
    const qreal midY = (start.y() + target.y()) * 0.5;
    const QPointF bendA(start.x(), midY);
    const QPointF bendB(target.x(), midY);
    const qreal firstLength = QLineF(start, bendA).length();
    const qreal secondLength = QLineF(bendA, bendB).length();
    const qreal thirdLength = QLineF(bendB, target).length();
    const qreal halfway = (firstLength + secondLength + thirdLength) * 0.5;

    QPainterPath path(start);
    path.lineTo(bendA);
    path.lineTo(bendB);
    path.lineTo(target);

    QPointF arrowTip;
    QPointF direction;
    if (halfway <= firstLength && firstLength > 0.0) {
        arrowTip = start + (bendA - start) * (halfway / firstLength);
        direction = QPointF(0.0, 1.0);
    } else if (halfway <= firstLength + secondLength && secondLength > 0.0) {
        arrowTip = bendA + (bendB - bendA) *
            ((halfway - firstLength) / secondLength);
        direction = bendB.x() >= bendA.x() ? QPointF(1.0, 0.0) : QPointF(-1.0, 0.0);
    } else if (thirdLength > 0.0) {
        arrowTip = bendB + (target - bendB) *
            ((halfway - firstLength - secondLength) / thirdLength);
        direction = QPointF(0.0, 1.0);
    } else {
        arrowTip = start;
        direction = QPointF(0.0, 1.0);
    }

    addArrow(path, arrowTip, direction);
    line->setBrush(Qt::NoBrush);
    line->setPath(path);
}

} // namespace workflow_editor_connections