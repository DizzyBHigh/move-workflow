#pragma once

class NodeItem;
class QPainter;
class QPointF;
class QRectF;

namespace workflow_node_play {

void paint(QPainter *painter, const NodeItem *node, const QRectF &rect, bool active);
bool contains(const NodeItem *node, const QPointF &scenePos);
void execute(const NodeItem *node, const char *workflowId);

} // namespace workflow_node_play
