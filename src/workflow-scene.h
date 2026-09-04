#pragma once

#include "workflow-node.h"

#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QList>
#include <QVector>

class QGraphicsRectItem;
class QGraphicsTextItem;

class EditorScene final : public QGraphicsScene {
    Q_OBJECT

public:
    explicit EditorScene(QObject *parent = nullptr);
    void setWorkflowId(const QString &workflowId);
    NodeItem *addNode(workflow_node_type_t type, const QString &name);
    NodeItem *selectedNode() const;
    QList<NodeItem *> selectedNodes() const;
    QList<NodeItem *> nodes() const;
    void deleteNode(NodeItem *node);
    void deleteSelectedNodes();
    void refreshConnectionsFor(NodeItem *node);
    void rebuildConnections();
    void updateConnections();
    void updateSceneBounds();
    bool editConnection(QGraphicsPathItem *line, const QString &type);

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
    struct MissingConnection { NodeItem *from = nullptr; QGraphicsPathItem *line = nullptr; QString type; QString targetId; };
    NodeItem *findNodeById(const char *id) const;
    NodeItem *nodeAt(const QPointF &scenePos) const;
    QGraphicsPathItem *connectionAt(const QPointF &scenePos) const;
    Connection *findConnection(QGraphicsPathItem *line);
    void addRelationshipLines(NodeItem *from, size_t count, const char ids[][WORKFLOW_MAX_NAME], const QString &type);
    void rebuildMissingNode();
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
    QVector<MissingConnection> missingConnections_;
    QGraphicsRectItem *missingNode_ = nullptr;
    QGraphicsTextItem *missingNodeLabel_ = nullptr;
    NodeItem *dragSource_ = nullptr;
    QGraphicsPathItem *dragPreview_ = nullptr;
    bool draggingConnection_ = false;
    QString workflowId_;
};
