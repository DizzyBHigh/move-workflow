#include "workflow-editor-connections.hpp"

#include "workflow-node.h"
#include "workflow-scene.h"

#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QLineF>
#include <QtMath>
#include <limits>

namespace workflow_editor_connections {

NodeItem *node_at(EditorScene *scene, const QPointF &scene_pos)
{
    if (!scene)
        return nullptr;

    const QList<QGraphicsItem *> hits = scene->items(scene_pos, Qt::IntersectsItemShape,
                                                      Qt::DescendingOrder);
    for (QGraphicsItem *item : hits) {
        while (item && !dynamic_cast<NodeItem *>(item))
            item = item->parentItem();
        if (auto *node = dynamic_cast<NodeItem *>(item))
            return node;
    }

    NodeItem *nearest = nullptr;
    qreal nearestDistance = std::numeric_limits<qreal>::max();
    for (NodeItem *node : scene->nodes()) {
        if (!node || !node->sceneBoundingRect().contains(scene_pos))
            continue;
        const qreal distance = QLineF(node->sceneBoundingRect().center(), scene_pos).length();
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearest = node;
        }
    }
    return nearest;
}

QString target_id(NodeItem *node)
{
    return node ? node->id() : QString();
}

void update_path(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
{
    if (!line || !from || !to)
        return;

    const QPointF start = from->sceneBoundingRect().center();
    const QPointF end = to->sceneBoundingRect().center();
    const qreal dx = end.x() - start.x();
    const qreal dy = end.y() - start.y();
    QPainterPath path(start);
    const QPointF control1 = start + QPointF(dx * 0.35, dy * 0.05);
    const QPointF control2 = end - QPointF(dx * 0.35, dy * 0.05);
    path.cubicTo(control1, control2, end);

    const qreal angle = qAtan2(end.y() - control2.y(), end.x() - control2.x());
    const qreal size = 12.0;
    const QPointF left(end.x() - qCos(angle - M_PI / 6.0) * size,
                       end.y() - qSin(angle - M_PI / 6.0) * size);
    const QPointF right(end.x() - qCos(angle + M_PI / 6.0) * size,
                        end.y() - qSin(angle + M_PI / 6.0) * size);
    path.moveTo(left);
    path.lineTo(end);
    path.lineTo(right);
    path.closeSubpath();
    line->setPath(path);
}

} // namespace workflow_editor_connections