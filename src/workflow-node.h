#pragma once

#include "workflow-model.h"

#include <QGraphicsRectItem>
#include <QPointF>

struct EditorNode {
    workflow_node_t workflow{};
    int numeric_id = 0;
    QPointF position;
};

class NodeItem final : public QGraphicsRectItem {
public:
    NodeItem(EditorNode node, QGraphicsItem *parent = nullptr);

    QString id() const;
    QString nodeName() const;
    workflow_node_t *workflowNode();
    const workflow_node_t *workflowNode() const;

    void refreshDisplay();
    bool isOnConnectionHandle(const QPointF &scenePos) const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

private:
    static constexpr qreal nodeWidth = 300.0;
    static constexpr qreal minimumHeight = 142.0;
    static constexpr qreal dragBarHeight = 10.0;
    static constexpr qreal dragBarHitHeight = 22.0;

    void updateGeometryForText();
    void refreshStyle();

    EditorNode node_;
    QGraphicsTextItem *title_ = nullptr;
    QGraphicsTextItem *type_ = nullptr;
    QGraphicsTextItem *details_ = nullptr;
};
