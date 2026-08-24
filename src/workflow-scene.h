#pragma once

#include "workflow-node.h"

#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QList>
#include <QVector>

class EditorScene final : public QGraphicsScene {
    Q_OBJECT

public:
    explicit EditorScene(QObject *parent = nullptr);

    NodeItem *addNode(workflow_node_type_t type, const QString &name);
    NodeItem *selectedNode() const;
    QList<NodeItem *> nodes() const;

    void deleteNode(NodeItem *node);
    void refreshConnectionsFor(NodeItem *node);
    void rebuildConnections();
    void updateConnections();
    void updateSceneBounds();

signals:
    void nodeDoubleClicked(NodeItem *node);

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    struct Connection {
        NodeItem *from = nullptr;
        NodeItem *to = nullptr;
        QGraphicsPathItem *line = nullptr;
        QString type;
    };

    NodeItem *findNodeById(const char *id) const;

    void addRelationshipLines(
        NodeItem *from,
        size_t count,
        const char ids[][WORKFLOW_MAX_NAME],
        const QString &type);

    static void updateConnection(
        QGraphicsPathItem *line,
        NodeItem *from,
        NodeItem *to);

    int nextId_ = 0;
    QVector<NodeItem *> nodes_;
    QVector<Connection> connections_;
};
