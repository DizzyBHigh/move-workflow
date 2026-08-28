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

static QPointF edgePoint(NodeItem *node, const QPointF &other)
{
    const QRectF rect = node->sceneBoundingRect();
    const QPointF center = rect.center();
    const qreal dx = other.x() - center.x();
    const qreal dy = other.y() - center.y();

    if (qAbs(dx) > qAbs(dy))
        return QPointF(dx >= 0 ? rect.right() : rect.left(), center.y());
    return QPointF(center.x(), dy >= 0 ? rect.bottom() : rect.top());
}

static void addPort(QPainterPath &path, const QPointF &point)
{
    path.addEllipse(point, 3.0, 3.0);
}

void update_path(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
{
    if (!line || !from || !to)
        return;

    const QRectF fromRect = from->sceneBoundingRect();
    const QRectF toRect = to->sceneBoundingRect();
    const QPointF start = edgePoint(from, toRect.center());
    const QPointF target = edgePoint(to, fromRect.center());

    const bool horizontal = qAbs(toRect.center().x() - fromRect.center().x()) >
                            qAbs(toRect.center().y() - fromRect.center().y());
    const qreal midX = (start.x() + target.x()) * 0.5;
    const qreal midY = (start.y() + target.y()) * 0.5;

    QPainterPath path(start);
    if (horizontal) {
        path.lineTo(QPointF(midX, start.y()));
        path.lineTo(QPointF(midX, target.y()));
    } else {
        path.lineTo(QPointF(start.x(), midY));
        path.lineTo(QPointF(target.x(), midY));
    }
    path.lineTo(target);

    const QPointF arrowTip = horizontal ? QPointF(midX, start.y())
                                        : QPointF(start.x(), midY);
    const QPointF direction = horizontal
        ? QPointF(target.x() >= start.x() ? 1.0 : -1.0, 0.0)
        : QPointF(0.0, target.y() >= start.y() ? 1.0 : -1.0);
    const QPointF side(-direction.y(), direction.x());
    const qreal size = 7.0;
    const qreal halfWidth = 4.0;

    QPainterPath arrow;
    arrow.moveTo(arrowTip - direction * size + side * halfWidth);
    arrow.lineTo(arrowTip);
    arrow.lineTo(arrowTip - direction * size - side * halfWidth);
    path.addPath(arrow);

    addPort(path, start);
    addPort(path, target);
    line->setBrush(Qt::NoBrush);
    line->setPath(path);
}

} // namespace workflow_editor_connections