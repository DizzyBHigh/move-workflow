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
    void workflowChanged();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    struct Connection { NodeItem *from = nullptr; NodeItem *to = nullptr; QGraphicsPathItem *line = nullptr; QString type; };
    NodeItem *findNodeById(const char *id) const;
    NodeItem *nodeAt(const QPointF &scenePos) const;
    void addRelationshipLines(NodeItem *from, size_t count, const char ids[][WORKFLOW_MAX_NAME], const QString &type);
    bool addNodeId(size_t &count, char ids[][WORKFLOW_MAX_NAME], const QString &id);
    bool hasNodeId(size_t count, const char ids[][WORKFLOW_MAX_NAME], const QString &id) const;
    void connectNodes(NodeItem *source, NodeItem *target);
    void connectTriggerToAction(NodeItem *trigger, NodeItem *action);
    void connectActionToAction(NodeItem *source, NodeItem *target, const QString &type);
    void finishConnectionDrag(const QPointF &scenePos);
    static void updateConnection(QGraphicsPathItem *line, NodeItem *from, NodeItem *to);
    int nextId_ = 0;
    QVector<NodeItem *> nodes_;
    QVector<Connection> connections_;
    NodeItem *dragSource_ = nullptr;
    QGraphicsPathItem *dragPreview_ = nullptr;
    bool draggingConnection_ = false;
};
