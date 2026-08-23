#include "workflow-node.h"

#include <QGraphicsTextItem>
#include <QPen>

#include <QString>

#include <utility>

namespace {

static QString read_text(const char *value)
{
    return value ? QString::fromUtf8(value) : QString();
}

} // namespace

NodeItem::NodeItem(EditorNode node, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), node_(std::move(node))
{
    setRect(0, 0, nodeWidth, minimumHeight);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setPos(node_.position);
    refreshStyle();

    title_ = new QGraphicsTextItem(read_text(node_.workflow.name), this);
    title_->setDefaultTextColor(Qt::white);
    title_->setTextWidth(nodeWidth - 28);
    title_->setPos(14, 10);

    type_ = new QGraphicsTextItem(this);
    type_->setDefaultTextColor(QColor(175, 185, 200));
    type_->setPos(14, 34);

    details_ = new QGraphicsTextItem(this);
    details_->setDefaultTextColor(QColor(210, 215, 220));
    details_->setTextWidth(nodeWidth - 28);
    details_->setPos(14, 56);
    refreshDisplay();
}

QString NodeItem::id() const
{
    return read_text(node_.workflow.id);
}

QString NodeItem::nodeName() const
{
    return read_text(node_.workflow.name);
}

workflow_node_t *NodeItem::workflowNode()
{
    return &node_.workflow;
}

const workflow_node_t *NodeItem::workflowNode() const
{
    return &node_.workflow;
}

void NodeItem::refreshDisplay()
{
    if (!title_ || !type_ || !details_)
        return;

    title_->setPlainText(read_text(node_.workflow.name));
    type_->setPlainText(node_.workflow.type == WORKFLOW_NODE_TRIGGER ? "TRIGGER" : "ACTION");

    if (node_.workflow.type == WORKFLOW_NODE_TRIGGER) {
        const QString trigger = QString::fromUtf8(workflow_trigger_type_name(node_.workflow.trigger.type));
        QString target;
        if (node_.workflow.trigger.type == WORKFLOW_TRIGGER_FILTER_ENABLE) {
            target = QString("%1 / %2")
                         .arg(read_text(node_.workflow.trigger.scene_name),
                              read_text(node_.workflow.trigger.filter_name));
        } else if (node_.workflow.trigger.type == WORKFLOW_TRIGGER_SOURCE_VISIBILITY ||
                   node_.workflow.trigger.type == WORKFLOW_TRIGGER_SOURCE_MUTE ||
                   node_.workflow.trigger.type == WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK ||
                   node_.workflow.trigger.type == WORKFLOW_TRIGGER_SOURCE_HOTKEY) {
            target = read_text(node_.workflow.trigger.scene_name);
        } else if (node_.workflow.trigger.type == WORKFLOW_TRIGGER_FRONTEND_ACTION) {
            target = read_text(node_.workflow.trigger.action);
        } else if (node_.workflow.trigger.type == WORKFLOW_TRIGGER_FRONTEND_HOTKEY) {
            target = read_text(node_.workflow.trigger.hotkey);
        }
        details_->setPlainText(QString("Trigger: %1%2\nConnections: %3")
                                   .arg(trigger,
                                        target.isEmpty() ? QString() : QString("\n%1").arg(target))
                                   .arg((qulonglong)(node_.workflow.end_node_count +
                                                     node_.workflow.simultaneous_node_count +
                                                     node_.workflow.next_node_count)));
    } else {
        const QString source = read_text(node_.workflow.action.scene_name);
        const QString filter = read_text(node_.workflow.action.filter_name);
        details_->setPlainText(QString("%1\n%2\nDelay %3 ms   Duration %4 ms\nSimultaneous %5   End %6   Next %7")
                                   .arg(source.isEmpty() ? "No source selected" : source,
                                        filter.isEmpty() ? "No Move filter selected" : filter)
                                   .arg((qulonglong)node_.workflow.start_delay.delay_ms)
                                   .arg((qulonglong)node_.workflow.duration.duration_ms)
                                   .arg((qulonglong)node_.workflow.simultaneous_node_count)
                                   .arg((qulonglong)node_.workflow.end_node_count)
                                   .arg((qulonglong)node_.workflow.next_node_count));
    }

    updateGeometryForText();
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged)
        node_.position = value.toPointF();
    return QGraphicsRectItem::itemChange(change, value);
}

void NodeItem::updateGeometryForText()
{
    const qreal contentHeight = details_->boundingRect().height();
    const qreal desiredHeight = qMax(minimumHeight, 56.0 + contentHeight + 14.0);
    if (!qFuzzyCompare(rect().height(), desiredHeight)) {
        prepareGeometryChange();
        setRect(0, 0, nodeWidth, desiredHeight);
    }
}

void NodeItem::refreshStyle()
{
    if (node_.workflow.type == WORKFLOW_NODE_TRIGGER) {
        setBrush(QColor(35, 70, 85));
        setPen(QPen(QColor(70, 180, 220), 2));
    } else {
        setBrush(QColor(42, 45, 50));
        setPen(QPen(QColor(110, 120, 135), 1));
    }
}
