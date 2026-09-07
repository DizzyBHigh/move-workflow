#pragma once

class NodeItem;
class QPainter;
class QPointF;

namespace workflow_node_play {

void paint(QPainter *painter, const NodeItem *node, const QRectF &rect);
bool contains(const NodeItem *node, const QPointF &scenePos);
void execute(const NodeItem *node);

} // namespace workflow_node_play
