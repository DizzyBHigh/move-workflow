#include "workflow-editor-connections.hpp"

#include "workflow-node.h"
#include "workflow-scene.h"

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QLineF>
#include <QtMath>

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

void update_path(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
{
    if (!line || !from || !to)
        return;

    const QPointF start = sourcePoint(from);
    const QPointF target = targetPoint(to);
    const qreal direction = target.y() >= start.y() ? 1.0 : -1.0;
    const qreal distance = qAbs(target.y() - start.y());
    const qreal curve = qMax(40.0, distance * 0.35);
    const QPointF control1 = start + QPointF(0.0, direction * curve);
    const QPointF control2 = target - QPointF(0.0, direction * curve);

    QPainterPath path(start);
    path.cubicTo(control1, control2, target);

    const qreal angle = qAtan2(target.y() - control2.y(), target.x() - control2.x());
    const qreal size = 12.0;
    const QPointF left(target.x() - qCos(angle - M_PI / 6.0) * size,
                       target.y() - qSin(angle - M_PI / 6.0) * size);
    const QPointF right(target.x() - qCos(angle + M_PI / 6.0) * size,
                        target.y() - qSin(angle + M_PI / 6.0) * size);
    QPainterPath arrow;
    arrow.moveTo(target);
    arrow.lineTo(left);
    arrow.lineTo(right);
    arrow.closeSubpath();
    path.addPath(arrow);

    line->setBrush(line->pen().color());
    line->setPath(path);
}

} // namespace workflow_editor_connections
