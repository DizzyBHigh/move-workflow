#include "workflow-scene.h"
#include "workflow-connection-editor.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QTransform>

namespace {
struct ConnectionEditContext { EditorScene *scene = nullptr; QGraphicsPathItem *line = nullptr; };
static void handle_connection_edit(void *context, const QString &type)
{
    auto *edit = static_cast<ConnectionEditContext *>(context);
    if (edit && edit->scene) edit->scene->editConnection(edit->line, type);
}
}

void EditorScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (QGraphicsPathItem *line = connectionAt(event->scenePos())) {
            if (Connection *connection = findConnection(line)) {
                ConnectionEditContext context{this, line};
                WorkflowConnectionEditor::showMenu(line, event->screenPos(), connection->type,
                                                   handle_connection_edit, &context);
                event->accept(); return;
            }
        }
        NodeItem *node = nodeAt(event->scenePos());
        if (node && node->isOnConnectionHandle(event->scenePos())) {
            draggingConnection_ = true; dragSource_ = node;
            dragPreview_ = new QGraphicsPathItem;
            dragPreview_->setPen(QPen(QColor(235, 240, 245), 2, Qt::DashLine));
            dragPreview_->setZValue(10); addItem(dragPreview_);
            QPainterPath path(event->scenePos()); path.lineTo(event->scenePos());
            dragPreview_->setPath(path); event->accept(); return;
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

void EditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (draggingConnection_ && dragPreview_) {
        const QPointF start = dragPreview_->path().elementAt(0);
        const QPointF end = event->scenePos();
        const qreal dx = end.x() - start.x(), dy = end.y() - start.y();
        QPainterPath path(start);
        path.cubicTo(start + QPointF(dx * 0.35, dy * 0.05), end - QPointF(dx * 0.35, dy * 0.05), end);
        dragPreview_->setPath(path); event->accept(); return;
    }
    QGraphicsScene::mouseMoveEvent(event);
    updateConnections();
}

void EditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (draggingConnection_ && event->button() == Qt::LeftButton) {
        finishConnectionDrag(event->scenePos()); event->accept(); return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
    updateConnections();
}

void EditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    while (item && !dynamic_cast<NodeItem *>(item)) item = item->parentItem();
    if (auto *node = dynamic_cast<NodeItem *>(item)) emit nodeDoubleClicked(node);
    QGraphicsScene::mouseDoubleClickEvent(event);
}