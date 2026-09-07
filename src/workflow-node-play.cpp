#include "workflow-node-play.hpp"

#include "workflow-node.h"
#include "workflow-engine-service.h"

#include <QPainter>
#include <QRectF>
#include <QString>

namespace workflow_node_play {

namespace {
constexpr qreal kButtonWidth = 30.0;
constexpr qreal kButtonHeight = 26.0;
constexpr qreal kRightMargin = 10.0;
constexpr qreal kTopMargin = 8.0;

QRectF button_rect(const QRectF &rect)
{
    return QRectF(rect.right() - kRightMargin - kButtonWidth,
                  rect.top() + kTopMargin, kButtonWidth, kButtonHeight);
}
}

void paint(QPainter *painter, const NodeItem *, const QRectF &rect, bool active)
{
    if (!painter)
        return;

    const QRectF button = button_rect(rect);
    painter->save();
    painter->setPen(QPen(active ? QColor(72, 190, 92) : QColor(90, 105, 120), 1));
    painter->setBrush(active ? QColor(35, 145, 62) : QColor(32, 42, 52));
    painter->drawRoundedRect(button, 4, 4);
    painter->setPen(QColor(225, 230, 235));
    painter->setFont(QFont(QStringLiteral("Segoe UI"), 12, QFont::Normal));
    painter->drawText(button, Qt::AlignCenter, QStringLiteral("▶"));
    painter->restore();
}

bool contains(const NodeItem *node, const QPointF &scenePos)
{
    if (!node)
        return false;
    const QPointF local = node->mapFromScene(scenePos);
    return button_rect(node->rect()).contains(local);
}

void execute(const NodeItem *node, const char *workflowId)
{
    if (!node || !workflowId || !workflowId[0])
        return;

    const workflow_node_t *workflowNode = node->workflowNode();
    if (!workflowNode || !workflowNode->id[0])
        return;

    workflow_engine_service_run_from_node(workflowId, workflowNode->id);
}

} // namespace workflow_node_play
