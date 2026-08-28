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
    const qreal midY = (start.y() + target.y()) * 0.5;
    const QPointF midStart(start.x(), midY);
    const QPointF midTarget(target.x(), midY);

    QPainterPath path(start);
    path.lineTo(midStart);
    path.lineTo(midTarget);
    path.lineTo(target);

    const QPointF arrowTip = QPointF((midStart.x() + midTarget.x()) * 0.5, midY);
    const qreal direction = midTarget.x() >= midStart.x() ? 1.0 : -1.0;
    const qreal size = 7.0;
    const qreal halfWidth = 4.0;
    QPainterPath arrow;
    arrow.moveTo(arrowTip);
    arrow.lineTo(arrowTip - QPointF(direction * size, halfWidth));
    arrow.lineTo(arrowTip - QPointF(direction * size, -halfWidth));
    arrow.closeSubpath();
    path.addPath(arrow);

    line->setBrush(line->pen().color());
    line->setPath(path);
}

} // namespace workflow_editor_connections