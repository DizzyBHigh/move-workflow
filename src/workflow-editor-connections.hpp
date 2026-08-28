#pragma once

#include <QPointF>
#include <QString>

class EditorScene;
class NodeItem;
class QGraphicsPathItem;

namespace workflow_editor_connections {

NodeItem *node_at(EditorScene *scene, const QPointF &scene_pos);
void update_path(QGraphicsPathItem *line, NodeItem *from, NodeItem *to);
QString target_id(NodeItem *node);

} // namespace workflow_editor_connections
