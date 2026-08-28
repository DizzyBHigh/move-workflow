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

    NodeItem *nearest = nullptr;
    qreal nearestDistance = std::numeric_limits<qreal>::max();
    for (NodeItem *node : scene->nodes()) {
        if (!node || !node->contains(node->mapFromScene(scene_pos)))
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

static QPointF edgePoint(NodeItem *node, const QPointF &toward)
{
    const QRectF rect = node->sceneBoundingRect();
    const QPointF center = rect.center();
    const qreal dx = toward.x() - center.x();
    const qreal dy = toward.y() - center.y();
    if (qFuzzyIsNull(dx) && qFuzzyIsNull(dy))
        return center;

    const qreal tx = qFuzzyIsNull(dx) ? std::numeric_limits<qreal>::max()
                                      : (rect.width() * 0.5) / qAbs(dx);
    const qreal ty = qFuzzyIsNull(dy) ? std::numeric_limits<qreal>::max()
                                      : (rect.height() * 0.5) / qAbs(dy);
    const qreal scale = qMin(tx, ty);
    return center + QPointF(dx * scale, dy * scale);
}

void update_path(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
{
    if (!line || !from || !to)
        return;

    const QPointF start = edgePoint(from, to->sceneBoundingRect().center());
    const QPointF end = edgePoint(to, from->sceneBoundingRect().center());
    const qreal dx = end.x() - start.x();
    const qreal dy = end.y() - start.y();
    const QPointF control1 = start + QPointF(dx * 0.35, dy * 0.05);
    const QPointF control2 = end - QPointF(dx * 0.35, dy * 0.05);
    QPainterPath path(start);
    path.cubicTo(control1, control2, end);

    const qreal angle = qAtan2(end.y() - control2.y(), end.x() - control2.x());
    const qreal size = 12.0;
    const QPointF left(end.x() - qCos(angle - M_PI / 6.0) * size,
                       end.y() - qSin(angle - M_PI / 6.0) * size);
    const QPointF right(end.x() - qCos(angle + M_PI / 6.0) * size,
                        end.y() - qSin(angle + M_PI / 6.0) * size);
    QPainterPath arrow;
    arrow.moveTo(end);
    arrow.lineTo(left);
    arrow.lineTo(right);
    arrow.closeSubpath();
    path.addPath(arrow);
    line->setBrush(line->pen().color());
    line->setPath(path);
}

} // namespace workflow_editor_connections
