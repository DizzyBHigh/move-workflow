#include "workflow-scene.h"
#include "workflow-connection-editor.h"

#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QTransform>

#include <cmath>
#include <cstring>
#include <utility>

namespace {

static void copy_text(char *destination, size_t capacity, const QString &value)
{
    if (!destination || capacity == 0)
        return;
    const QByteArray bytes = value.toUtf8();
    std::strncpy(destination, bytes.constData(), capacity - 1);
    destination[capacity - 1] = '\0';
}

static bool add_node_id(size_t &count, char ids[][WORKFLOW_MAX_NAME], const QString &id)
{
    const QByteArray bytes = id.toUtf8();
    for (size_t i = 0; i < count; ++i) {
        if (std::strcmp(ids[i], bytes.constData()) == 0)
            return false;
    }
    if (count >= WORKFLOW_MAX_NODES)
        return false;
    std::strncpy(ids[count], bytes.constData(), WORKFLOW_MAX_NAME - 1);
    ids[count][WORKFLOW_MAX_NAME - 1] = '\0';
    ++count;
    return true;
}

static void remove_id(size_t &count, char ids[][WORKFLOW_MAX_NAME], const QString &id)
{
    const QByteArray wanted = id.toUtf8();
    size_t write = 0;
    for (size_t read = 0; read < count; ++read) {
        if (std::strcmp(ids[read], wanted.constData()) != 0) {
            if (write != read)
                std::memcpy(ids[write], ids[read], WORKFLOW_MAX_NAME);
            ++write;
        }
    }
    count = write;
}

struct ConnectionEditContext {
    EditorScene *scene = nullptr;
    QGraphicsPathItem *line = nullptr;
};

static void handle_connection_edit(void *context, const QString &type)
{
    auto *edit = static_cast<ConnectionEditContext *>(context);
    if (edit && edit->scene)
        edit->scene->editConnection(edit->line, type);
}

} // namespace

EditorScene::EditorScene(QObject *parent) : QGraphicsScene(parent)
{
    connect(this, &QGraphicsScene::changed, this, [this] { updateConnections(); });
}

NodeItem *EditorScene::addNode(workflow_node_type_t type, const QString &name)
{
    EditorNode node;
    node.numeric_id = ++nextId_;
    copy_text(node.workflow.id, WORKFLOW_MAX_NAME, QString("node-%1").arg(node.numeric_id));
    copy_text(node.workflow.name, WORKFLOW_MAX_NAME, name);
    node.workflow.type = type;
    node.workflow.trigger_count = 0;
    node.workflow.duration.mode = WORKFLOW_OVERRIDE;
    node.workflow.start_delay.mode = WORKFLOW_OVERRIDE;
    node.workflow.end_delay.mode = WORKFLOW_OVERRIDE;
    node.workflow.simultaneous_actions_mode = WORKFLOW_OVERRIDE;
    node.workflow.next_actions_mode = WORKFLOW_OVERRIDE;
    auto *item = new NodeItem(node);
    addItem(item);
    nodes_.push_back(item);
    updateSceneBounds();
    return item;
}

NodeItem *EditorScene::selectedNode() const
{
    for (QGraphicsItem *item : selectedItems())
        if (auto *node = dynamic_cast<NodeItem *>(item))
            return node;
    return nullptr;
}

QList<NodeItem *> EditorScene::nodes() const { return nodes_; }

void EditorScene::deleteNode(NodeItem *node)
{
    if (!node)
        return;
    const QString id = node->id();
    for (NodeItem *candidate : nodes_) {
        auto *wf = candidate->workflowNode();
        remove_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, id);
        remove_id(wf->next_node_count, wf->next_node_ids, id);
        remove_id(wf->shortcut_node_count, wf->shortcut_node_ids, id);
    }
    nodes_.removeAll(node);
    removeItem(node);
    delete node;
    rebuildConnections();
    updateSceneBounds();
}

void EditorScene::refreshConnectionsFor(NodeItem *)
{
    rebuildConnections();
    updateSceneBounds();
}

void EditorScene::rebuildConnections()
{
    for (const Connection &connection : std::as_const(connections_)) {
        removeItem(connection.line);
        delete connection.line;
    }
    connections_.clear();

    for (NodeItem *from : nodes_) {
        const auto *wf = from->workflowNode();
        addRelationshipLines(from, wf->simultaneous_node_count,
                             wf->simultaneous_node_ids, "Simultaneous");
        addRelationshipLines(from, wf->next_node_count, wf->next_node_ids, "Next Action");
        addRelationshipLines(from, wf->shortcut_node_count, wf->shortcut_node_ids, "Shortcut");
    }
    updateConnections();
}

void EditorScene::updateConnections()
{
    for (const Connection &connection : std::as_const(connections_))
        updateConnection(connection.line, connection.from, connection.to);

    if (draggingConnection_ && dragPreview_ && dragSource_) {
        const QPointF start = dragPreview_->path().elementAt(0);
        const QPointF end = dragPreview_->path().currentPosition();
        const qreal dx = end.x() - start.x();
        const qreal dy = end.y() - start.y();
        QPainterPath path(start);
        path.cubicTo(start + QPointF(dx * 0.35, dy * 0.05),
                     end - QPointF(dx * 0.35, dy * 0.05), end);
        dragPreview_->setPath(path);
    }
    updateSceneBounds();
}

void EditorScene::updateSceneBounds()
{
    if (nodes_.isEmpty()) {
        setSceneRect(0, 0, 2000, 1400);
        return;
    }
    QRectF bounds;
    bool valid = false;
    for (NodeItem *node : nodes_) {
        const QRectF rect = node->sceneBoundingRect();
        bounds = valid ? bounds.united(rect) : rect;
        valid = true;
    }
    constexpr qreal margin = 160.0;
    constexpr qreal minimumWidth = 900.0;
    constexpr qreal minimumHeight = 600.0;
    bounds.adjust(-margin, -margin, margin, margin);
    if (bounds.width() < minimumWidth) {
        const qreal extra = (minimumWidth - bounds.width()) * 0.5;
        bounds.adjust(-extra, 0, extra, 0);
    }
    if (bounds.height() < minimumHeight) {
        const qreal extra = (minimumHeight - bounds.height()) * 0.5;
        bounds.adjust(0, -extra, 0, extra);
    }
    setSceneRect(bounds);
}

QGraphicsPathItem *EditorScene::connectionAt(const QPointF &scenePos) const
{
    QPainterPathStroker stroker;
    stroker.setWidth(12.0);
    for (auto it = connections_.crbegin(); it != connections_.crend(); ++it) {
        if (!it->line)
            continue;
        const QPointF localPos = it->line->mapFromScene(scenePos);
        if (stroker.createStroke(it->line->path()).contains(localPos))
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

bool EditorScene::editConnection(QGraphicsPathItem *line, const QString &type)
{
    Connection *connection = findConnection(line);
    if (!connection)
        return false;

    auto *wf = connection->from->workflowNode();
    const QString target = connection->to->id();
    remove_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, target);
    remove_id(wf->next_node_count, wf->next_node_ids, target);
    remove_id(wf->shortcut_node_count, wf->shortcut_node_ids, target);

    if (type == "__delete__") {
        rebuildConnections();
        emit workflowChanged();
        return true;
    }

    if (type == "Simultaneous")
        add_node_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, target);
    else if (type == "Next")
        add_node_id(wf->next_node_count, wf->next_node_ids, target);
    else if (type == "Shortcut")
        add_node_id(wf->shortcut_node_count, wf->shortcut_node_ids, target);
    else
        return false;

    connection->type = type == "Next" ? "Next Action" : type;
    connection->from->refreshDisplay();
    connection->to->refreshDisplay();
    rebuildConnections();
    emit workflowChanged();
    return true;
}

void EditorScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (QGraphicsPathItem *line = connectionAt(event->scenePos())) {
            if (Connection *connection = findConnection(line)) {
                ConnectionEditContext context{this, line};
                WorkflowConnectionEditor::showMenu(line, event->screenPos(), connection->type,
                                                   handle_connection_edit, &context);
                event->accept();
                return;
            }
        }
        NodeItem *node = nodeAt(event->scenePos());
        if (node && node->isOnConnectionHandle(event->scenePos())) {
            draggingConnection_ = true;
            dragSource_ = node;
            dragPreview_ = new QGraphicsPathItem;
            dragPreview_->setPen(QPen(QColor(235, 240, 245), 2, Qt::DashLine));
            dragPreview_->setZValue(10);
            addItem(dragPreview_);
            QPainterPath path(event->scenePos());
            path.lineTo(event->scenePos());
            dragPreview_->setPath(path);
            event->accept();
            return;
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

void EditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (draggingConnection_ && dragPreview_) {
        const QPointF start = dragPreview_->path().elementAt(0);
        const QPointF end = event->scenePos();
        const qreal dx = end.x() - start.x();
        const qreal dy = end.y() - start.y();
        QPainterPath path(start);
        path.cubicTo(start + QPointF(dx * 0.35, dy * 0.05),
                     end - QPointF(dx * 0.35, dy * 0.05), end);
        dragPreview_->setPath(path);
        event->accept();
        return;
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void EditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (draggingConnection_ && event->button() == Qt::LeftButton) {
        finishConnectionDrag(event->scenePos());
        event->accept();
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void EditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    while (item && !dynamic_cast<NodeItem *>(item))
        item = item->parentItem();
    if (auto *node = dynamic_cast<NodeItem *>(item))
        emit nodeDoubleClicked(node);
    QGraphicsScene::mouseDoubleClickEvent(event);
}

NodeItem *EditorScene::nodeAt(const QPointF &scenePos) const
{
    const QList<QGraphicsItem *> hits = items(scenePos, Qt::IntersectsItemShape,
                                               Qt::DescendingOrder, QTransform());
    for (QGraphicsItem *item : hits) {
        if (auto *node = dynamic_cast<NodeItem *>(item))
            return node;
        while (item) {
            item = item->parentItem();
            if (auto *node = dynamic_cast<NodeItem *>(item))
                return node;
        }
    }
    return nullptr;
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

void EditorScene::finishConnectionDrag(const QPointF &scenePos)
{
    NodeItem *source = dragSource_;
    NodeItem *target = nodeAt(scenePos);
    if (dragPreview_) {
        removeItem(dragPreview_);
        delete dragPreview_;
    }
    dragPreview_ = nullptr;
    dragSource_ = nullptr;
    draggingConnection_ = false;
    if (!source || !target || source == target)
        return;

    if (source->workflowNode()->type == WORKFLOW_NODE_ACTION &&
        target->workflowNode()->type == WORKFLOW_NODE_ACTION) {
        QMessageBox dialog(QMessageBox::Question, "Connect Actions",
                           "Choose how the destination Action should start.",
                           QMessageBox::NoButton, nullptr);
        QPushButton *simultaneous = dialog.addButton("Simultaneous", QMessageBox::AcceptRole);
        QPushButton *next = dialog.addButton("Next", QMessageBox::AcceptRole);
        QPushButton *shortcut = dialog.addButton("Shortcut", QMessageBox::AcceptRole);
        QPushButton *cancel = dialog.addButton("Cancel", QMessageBox::RejectRole);
        dialog.exec();
        if (dialog.clickedButton() == simultaneous)
            connectActionToAction(source, target, "Simultaneous");
        else if (dialog.clickedButton() == next)
            connectActionToAction(source, target, "Next");
        else if (dialog.clickedButton() == shortcut)
            connectActionToAction(source, target, "Shortcut");
        else if (dialog.clickedButton() == cancel)
            return;
    } else if (source->workflowNode()->type == WORKFLOW_NODE_TRIGGER &&
               target->workflowNode()->type == WORKFLOW_NODE_ACTION) {
        connectTriggerToAction(source, target);
    } else if (source->workflowNode()->type == WORKFLOW_NODE_ACTION &&
               target->workflowNode()->type == WORKFLOW_NODE_TRIGGER) {
        connectTriggerToAction(target, source);
    } else {
        return;
    }
    source->refreshDisplay();
    target->refreshDisplay();
    rebuildConnections();
    emit workflowChanged();
}

void EditorScene::connectTriggerToAction(NodeItem *trigger, NodeItem *action)
{
    if (!trigger || !action)
        return;
    add_node_id(trigger->workflowNode()->simultaneous_node_count,
                trigger->workflowNode()->simultaneous_node_ids, action->id());
}

void EditorScene::connectActionToAction(NodeItem *source, NodeItem *target, const QString &type)
{
    auto *wf = source->workflowNode();
    const QString targetId = target->id();
    remove_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, targetId);
    remove_id(wf->next_node_count, wf->next_node_ids, targetId);
    remove_id(wf->shortcut_node_count, wf->shortcut_node_ids, targetId);
    if (type == "Simultaneous")
        add_node_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, targetId);
    else if (type == "Next")
        add_node_id(wf->next_node_count, wf->next_node_ids, targetId);
    else
        add_node_id(wf->shortcut_node_count, wf->shortcut_node_ids, targetId);
}

void EditorScene::addRelationshipLines(NodeItem *from, size_t count,
                                       const char ids[][WORKFLOW_MAX_NAME],
                                       const QString &type)
{
    for (size_t i = 0; i < count; ++i) {
        NodeItem *to = findNodeById(ids[i]);
        if (!to || to == from)
            continue;
        auto *line = new QGraphicsPathItem;
        if (type == "Simultaneous")
            line->setPen(QPen(QColor(90, 190, 120), 2));
        else if (type == "Next Action")
            line->setPen(QPen(QColor(230, 170, 70), 2));
        else
            line->setPen(QPen(QColor(180, 120, 220), 2));
        line->setZValue(-1);
        addItem(line);
        connections_.push_back({from, to, line, type});
    }
}

void EditorScene::updateConnection(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
{
    if (!line || !from || !to)
        return;
    const QPointF start = from->sceneBoundingRect().center();
    const QPointF end = to->sceneBoundingRect().center();
    const qreal dx = end.x() - start.x();
    const qreal dy = end.y() - start.y();
    QPainterPath path(start);
    path.cubicTo(start + QPointF(dx * 0.35, dy * 0.05),
                 end - QPointF(dx * 0.35, dy * 0.05), end);
    line->setPath(path);
}
