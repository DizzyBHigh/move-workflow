#include "workflow-node.h"
#include "workflow-engine-service.h"
#include "workflow-node-timing-defaults.h"
#include "workflow-trigger-filter-instance.h"
#include "workflow-persistence.h"
#include <obs.h>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPen>
#include <QString>
#include <QStringList>
#include <utility>
#include <chrono>

namespace { static QString text(const char *v) { return v ? QString::fromUtf8(v) : QString(); } }
static int64_t runtime_now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

NodeItem::NodeItem(EditorNode n, QGraphicsItem *p) : QGraphicsRectItem(p), node_(std::move(n))
{
    setRect(0, 0, nodeWidth, minimumHeight); setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable); setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setPos(node_.position); refreshStyle();
    title_ = new QGraphicsTextItem(text(node_.workflow.name), this); title_->setDefaultTextColor(Qt::white);
    title_->setTextWidth(nodeWidth - 28); title_->setPos(14, 10);
    type_ = new QGraphicsTextItem(this); type_->setDefaultTextColor(QColor(175,185,200)); type_->setPos(14,34);
    details_ = new QGraphicsTextItem(this); details_->setDefaultTextColor(QColor(210,215,220));
    details_->setTextWidth(nodeWidth - 28); details_->setPos(14,56); refreshDisplay();
}

QString NodeItem::id() const { return text(node_.workflow.id); }
QString NodeItem::nodeName() const { return text(node_.workflow.name); }
workflow_node_t *NodeItem::workflowNode() { return &node_.workflow; }
const workflow_node_t *NodeItem::workflowNode() const { return &node_.workflow; }
void NodeItem::setWorkflowId(const QString &workflowId) { workflowId_ = workflowId; update(); }
void NodeItem::setWorkflowChangedCallback(std::function<void()> callback) { workflowChangedCallback_ = std::move(callback); }

void NodeItem::refreshDisplay()
{
    if (!title_ || !type_ || !details_) return;
    refreshStyle();
    title_->setPlainText(text(node_.workflow.name));
    type_->setPlainText(node_.workflow.type == WORKFLOW_NODE_TRIGGER ? "TRIGGER" : "ACTION");
    if (node_.workflow.type == WORKFLOW_NODE_TRIGGER) {
        QStringList refs;
        for (size_t i = 0; i < node_.workflow.trigger_count; ++i) {
            auto *f = workflow_trigger_filter_find(node_.workflow.triggers[i].source_uuid, node_.workflow.triggers[i].filter_uuid);
            auto *s = obs_get_source_by_uuid(node_.workflow.triggers[i].source_uuid);
            if (f && s) refs << QString("%1 / %2").arg(text(obs_source_get_name(s)), text(obs_source_get_name(f)));
            if (f) obs_source_release(f); if (s) obs_source_release(s);
        }
        details_->setPlainText(refs.isEmpty() ? "No Workflow Trigger Filters selected" : refs.join("\n"));
    } else {
        const QString source = text(node_.workflow.action.scene_name), filter = text(node_.workflow.action.filter_name);
        uint64_t sd=node_.workflow.start_delay.delay_ms, d=node_.workflow.duration.duration_ms, ed=node_.workflow.end_delay.delay_ms;
        const auto defaults = workflow_node_read_timing_defaults(node_.workflow.action.scene_name, node_.workflow.action.filter_name);
        if (defaults.valid) { if (node_.workflow.start_delay.mode==WORKFLOW_USE_EXISTING) sd=defaults.start_delay_ms;
            if (node_.workflow.duration.mode==WORKFLOW_USE_EXISTING) d=defaults.duration_ms;
            if (node_.workflow.end_delay.mode==WORKFLOW_USE_EXISTING) ed=defaults.end_delay_ms; }
        details_->setPlainText(QString("%1\n%2\nDelay %3 ms   Duration %4 ms   End %5 ms\nSimultaneous %6   End %7   Next %8")
            .arg(source.isEmpty()?"No source selected":source, filter.isEmpty()?"No Move filter selected":filter)
            .arg((qulonglong)sd).arg((qulonglong)d).arg((qulonglong)ed)
            .arg((qulonglong)node_.workflow.simultaneous_node_count).arg((qulonglong)node_.workflow.end_node_count).arg((qulonglong)node_.workflow.next_node_count));
    }
    updateGeometryForText(); update();
}

bool NodeItem::isOnConnectionHandle(const QPointF &p) const
{
    const QPointF l=mapFromScene(p);
    return QRectF(rect().left(),rect().bottom()-dragBarHitHeight,rect().width(),dragBarHitHeight).contains(l);
}

QVariant NodeItem::itemChange(GraphicsItemChange c, const QVariant &v)
{
    if (c==ItemPositionHasChanged) {
        node_.position=v.toPointF();
        if (workflowChangedCallback_) workflowChangedCallback_();
    }
    return QGraphicsRectItem::itemChange(c,v);
}

void NodeItem::paint(QPainter *p, const QStyleOptionGraphicsItem *o, QWidget *w)
{
    QGraphicsRectItem::paint(p,o,w);
    const QRectF r(rect().left(),rect().bottom()-dragBarHeight,rect().width(),dragBarHeight);
    p->setPen(Qt::NoPen); p->setBrush(node_.workflow.type==WORKFLOW_NODE_TRIGGER?QColor(48,174,76):QColor(47,122,190)); p->drawRect(r);
    workflow_engine_node_runtime_t runtime{};
    const bool active = node_.workflow.type == WORKFLOW_NODE_ACTION && !workflowId_.isEmpty() &&
        workflow_engine_service_node_runtime(workflowId_.toUtf8().constData(), node_.workflow.id, &runtime);
    if (active) {
        type_->setVisible(false); details_->setVisible(false);
        const QRectF body(rect().left()+2, rect().top()+42, rect().width()-4,
                          rect().height()-44-dragBarHeight);
        p->setPen(Qt::NoPen); p->setBrush(QColor(29,39,49)); p->drawRect(body);
        const int64_t remaining = workflow_engine_node_runtime_remaining_ms(&runtime, runtime_now_ms());
        const QString phase = runtime.phase == WORKFLOW_NODE_PHASE_START_DELAY ? "START DELAY" :
                              runtime.phase == WORKFLOW_NODE_PHASE_EXECUTION ? "EXECUTION" : "END DELAY";
        p->setPen(QColor(220,225,230)); p->setFont(QFont(QStringLiteral("Segoe UI"), 12, QFont::DemiBold));
        p->drawText(body.adjusted(14,20,-14,-38), Qt::AlignCenter, phase);
        p->setFont(QFont(QStringLiteral("Segoe UI"), 28, QFont::Normal));
        p->drawText(body.adjusted(14,48,-14,-8), Qt::AlignCenter,
                    QString::number(remaining / 1000.0, 'f', 1) + " s");
    } else {
        type_->setVisible(true); details_->setVisible(true);
    }
}

void NodeItem::updateGeometryForText()
{
    const qreal h=qMax(minimumHeight,56.0+details_->boundingRect().height()+14.0);
    if (!qFuzzyCompare(rect().height(),h)) { prepareGeometryChange(); setRect(0,0,nodeWidth,h); }
}

void NodeItem::refreshStyle()
{
    QColor fill(23,31,40), border(70,88,105);
    const workflow_node_t &wf = node_.workflow;
    const bool unconfigured = wf.type == WORKFLOW_NODE_ACTION &&
        (wf.action.kind != WORKFLOW_CHANGE_SCENE
             ? (!wf.action.scene_name[0] || !wf.action.filter_name[0] || !wf.action.filter_id[0])
             : !wf.action.scene_name[0]);
    if (wf.type == WORKFLOW_NODE_TRIGGER) { fill=QColor(20,63,34); border=QColor(48,174,76); }
    else if (unconfigured) { fill=QColor(100,25,25); border=QColor(220,65,65); }
    else { fill=QColor(18,58,93); border=QColor(47,122,190); }
    setBrush(fill); setPen(QPen(border,2));
}
