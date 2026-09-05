#include "workflow-scene.h"
#include "workflow-editor-connections.hpp"

#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPen>

QGraphicsPathItem *EditorScene::connectionAt(const QPointF &scenePos) const
{
    QPainterPathStroker stroker;
    stroker.setWidth(12.0);
    for (auto it = connections_.crbegin(); it != connections_.crend(); ++it) {
        if (!it->line)
            continue;
        const QPointF local = it->line->mapFromScene(scenePos);
        if (stroker.createStroke(it->line->path()).contains(local))
            return it->line;
    }
    for (auto it = missingConnections_.crbegin(); it != missingConnections_.crend(); ++it) {
        if (!it->line)
            continue;
        const QPointF local = it->line->mapFromScene(scenePos);
        if (stroker.createStroke(it->line->path()).contains(local))
            return it->line;
    }
    return nullptr;
}

EditorScene::Connection *EditorScene::findConnection(QGraphicsPathItem *line)
{
    if (!line)
        return nullptr;
    for (Connection &connection : connections_)
        if (connection.line == line)
            return &connection;
    return nullptr;
}

NodeItem *EditorScene::nodeAt(const QPointF &scenePos) const
{
    return workflow_editor_connections::node_at(const_cast<EditorScene *>(this), scenePos);
}

NodeItem *EditorScene::findNodeById(const char *id) const
{
    if (!id)
        return nullptr;
    for (NodeItem *node : nodes_)
        if (node && node->id() == QString::fromUtf8(id))
            return node;
    return nullptr;
}

void EditorScene::addRelationshipLines(NodeItem *from, size_t count,
                                       const char ids[][WORKFLOW_MAX_NAME],
                                       const QString &type)
{
    for (size_t i = 0; i < count; ++i) {
        NodeItem *to = findNodeById(ids[i]);
        if (!to) {
            auto *line = new QGraphicsPathItem;
            line->setPen(QPen(QColor(220, 70, 70), 2, Qt::DashLine));
            line->setZValue(-1);
            addItem(line);
            missingConnections_.push_back({from, line, type, QString::fromUtf8(ids[i])});
            continue;
        }
        if (to == from)
            continue;
        auto *line = new QGraphicsPathItem;
        if (type == "Simultaneous")
            line->setPen(QPen(QColor(90, 190, 120), 2));
        else if (type == "Next Action")
            line->setPen(QPen(QColor(230, 170, 70), 2));
        else
            line->setPen(QPen(QColor(180, 120, 220), 2));
        line->setBrush(line->pen().color());
        line->setZValue(-1);
        addItem(line);
        connections_.push_back({from, to, line, type});
    }
}

void EditorScene::updateConnection(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
{
    workflow_editor_connections::update_path(line, from, to);
}
