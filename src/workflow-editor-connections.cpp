#include "workflow-editor-connections.hpp"

#include "workflow-node.h"

#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPolygonF>
#include <QtMath>

namespace workflow_editor_connections {

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
    path.cubicTo(start + QPointF(dx * 0.35, dy * 0.05),
                 end - QPointF(dx * 0.35, dy * 0.05), end);

    const QPointF tangent = end - (end - start) * 0.08;
    const qreal angle = qAtan2(end.y() - tangent.y(), end.x() - tangent.x());
    const qreal size = 8.0;
    QPolygonF arrow;
    arrow << end
          << end - QPointF(qCos(angle - M_PI / 6.0) * size,
                           qSin(angle - M_PI / 6.0) * size)
          << end - QPointF(qCos(angle + M_PI / 6.0) * size,
                           qSin(angle + M_PI / 6.0) * size);
    path.moveTo(end);
    path.lineTo(arrow.at(1));
    path.moveTo(end);
    path.lineTo(arrow.at(2));
    line->setPath(path);
}

} // namespace workflow_editor_connections
