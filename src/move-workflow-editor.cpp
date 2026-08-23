#include "move-workflow-editor.h"
#include "workflow-model.h"

#include <obs-frontend-api.h>
#include <obs.h>

#include <QAction>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <cstring>
#include <memory>

namespace {

static void copy_text(char *destination, size_t capacity, const QString &value)
{
    if (!destination || capacity == 0)
        return;
    const QByteArray bytes = value.toUtf8();
    std::strncpy(destination, bytes.constData(), capacity - 1);
    destination[capacity - 1] = '\0';
}

static QString read_text(const char *value)
{
    return value ? QString::fromUtf8(value) : QString();
}

static bool list_contains(const char ids[][WORKFLOW_MAX_NAME], size_t count, const QString &id)
{
    const QByteArray wanted = id.toUtf8();
    for (size_t i = 0; i < count; ++i) {
        if (std::strcmp(ids[i], wanted.constData()) == 0)
            return true;
    }
    return false;
}

static void set_node_list(size_t &count, char ids[][WORKFLOW_MAX_NAME], const QListWidget *list)
{
    count = 0;
    for (int i = 0; i < list->count() && count < WORKFLOW_MAX_LINKS; ++i) {
        const QListWidgetItem *item = list->item(i);
        if (!item->isSelected())
            continue;
        copy_text(ids[count], WORKFLOW_MAX_NAME, item->data(Qt::UserRole).toString());
        ++count;
    }
}

static workflow_move_kind_t workflow_kind_from_filter_id(const char *id)
{
    if (!id)
        return WORKFLOW_MOVE_ACTION;
    if (std::strcmp(id, "move_source_filter") == 0)
        return WORKFLOW_MOVE_SOURCE;
    if (std::strcmp(id, "move_source_swap_filter") == 0)
        return WORKFLOW_MOVE_SWAP;
    if (std::strcmp(id, "move_value_filter") == 0)
        return WORKFLOW_MOVE_VALUE;
    return WORKFLOW_MOVE_ACTION;
}

static bool is_supported_move_filter(const char *id)
{
    return id && (std::strcmp(id, "move_action_filter") == 0 ||
                  std::strcmp(id, "move_source_filter") == 0 ||
                  std::strcmp(id, "move_source_swap_filter") == 0 ||
                  std::strcmp(id, "move_value_filter") == 0);
}

struct EditorNode {
    workflow_node_t workflow{};
    int numeric_id = 0;
    QPointF position;
};

class NodeItem final : public QGraphicsRectItem {
public:
    NodeItem(EditorNode node, QGraphicsItem *parent = nullptr)
        : QGraphicsRectItem(parent), node_(std::move(node))
    {
        setRect(0, 0, 250, 126);
        setBrush(QColor(42, 45, 50));
        setPen(QPen(QColor(110, 120, 135), 1));
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges);
        setPos(node_.position);

        title_ = new QGraphicsTextItem(read_text(node_.workflow.name), this);
        title_->setDefaultTextColor(Qt::white);
        title_->setPos(12, 9);

        details_ = new QGraphicsTextItem(this);
        details_->setDefaultTextColor(QColor(185, 190, 200));
        details_->setPos(12, 35);
        refreshDisplay();
    }

    int numericId() const { return node_.numeric_id; }
    QString id() const { return read_text(node_.workflow.id); }
    QString nodeName() const { return read_text(node_.workflow.name); }
    workflow_node_t *workflowNode() { return &node_.workflow; }
    const workflow_node_t *workflowNode() const { return &node_.workflow; }

    void setNodeName(const QString &name)
    {
        copy_text(node_.workflow.name, WORKFLOW_MAX_NAME, name);
        title_->setPlainText(name);
        refreshDisplay();
    }

    void refreshDisplay()
    {
        if (!details_)
            return;

        const QString filter = read_text(node_.workflow.action.filter_name).isEmpty()
                                   ? "No Move action selected"
                                   : read_text(node_.workflow.action.filter_name);
        const QString kind = QString::fromUtf8(workflow_move_kind_name(node_.workflow.action.kind));
        const QString trigger = read_text(node_.workflow.trigger.action);
        const QString duration = node_.workflow.duration.mode == WORKFLOW_OVERRIDE
                                     ? QString("Duration: %1 ms").arg((qulonglong)node_.workflow.duration.duration_ms)
                                     : "Duration: existing";
        const QString end = QString("End: %1").arg((qulonglong)node_.workflow.end_node_count);
        const QString simultaneous = QString("Simultaneous: %1").arg((qulonglong)node_.workflow.simultaneous_node_count);
        const QString next = QString("Next: %1").arg((qulonglong)node_.workflow.next_node_count);
        const QString triggerLine = trigger.isEmpty() || trigger == "None" ? "Trigger: none" : QString("Trigger: %1").arg(trigger);

        details_->setPlainText(QString("%1\n%2\n%3\n%4\n%5  %6  %7")
                                   .arg(filter, kind, triggerLine, duration, end, simultaneous, next));
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if (change == ItemPositionHasChanged)
            node_.position = value.toPointF();
        return QGraphicsRectItem::itemChange(change, value);
    }

private:
    EditorNode node_;
    QGraphicsTextItem *title_ = nullptr;
    QGraphicsTextItem *details_ = nullptr;
};

class EditorScene final : public QGraphicsScene {
    Q_OBJECT
public:
    explicit EditorScene(QObject *parent = nullptr) : QGraphicsScene(parent)
    {
        connect(this, &QGraphicsScene::changed, this, [this] { updateConnectionGeometry(); });
    }

    NodeItem *addNode(const QString &name)
    {
        EditorNode node;
        node.numeric_id = ++nextId_;
        const QString id = QString("node-%1").arg(node.numeric_id);
        copy_text(node.workflow.id, WORKFLOW_MAX_NAME, id);
        copy_text(node.workflow.name, WORKFLOW_MAX_NAME, name);
        copy_text(node.workflow.trigger.action, WORKFLOW_MAX_NAME, "None");
        node.workflow.duration.mode = WORKFLOW_USE_EXISTING;
        node.workflow.start_delay.mode = WORKFLOW_USE_EXISTING;
        node.workflow.end_actions_mode = WORKFLOW_OVERRIDE;
        node.workflow.start_trigger_mode = WORKFLOW_USE_EXISTING;
        node.workflow.stop_trigger_mode = WORKFLOW_USE_EXISTING;
        node.workflow.simultaneous_actions_mode = WORKFLOW_OVERRIDE;
        node.workflow.next_actions_mode = WORKFLOW_OVERRIDE;
        node.workflow.next_move_on_mode = WORKFLOW_USE_EXISTING;
        node.position = QPointF(80 + ((node.numeric_id - 1) % 4) * 290,
                                80 + ((node.numeric_id - 1) / 4) * 170);

        auto *item = new NodeItem(std::move(node));
        addItem(item);
        nodes_.push_back(item);
        updateSceneBounds();
        return item;
    }

    NodeItem *selectedNode() const
    {
        for (QGraphicsItem *item : selectedItems()) {
            if (auto *node = dynamic_cast<NodeItem *>(item))
                return node;
        }
        return nullptr;
    }

    QList<NodeItem *> nodes() const { return nodes_; }

    void refreshConnectionsFor(NodeItem *changedNode)
    {
        Q_UNUSED(changedNode);
        rebuildConnections();
        updateSceneBounds();
    }

    void rebuildConnections()
    {
        for (Connection &connection : connections_) {
            removeItem(connection.line);
            delete connection.line;
        }
        connections_.clear();

        for (NodeItem *from : nodes_) {
            const workflow_node_t *wf = from->workflowNode();
            addRelationshipLines(from, wf->end_node_count, wf->end_node_ids, "End Action");
            addRelationshipLines(from, wf->simultaneous_node_count, wf->simultaneous_node_ids, "Simultaneous");
            addRelationshipLines(from, wf->next_node_count, wf->next_node_ids, "Next Action");
        }
        updateConnections();
    }

    void updateConnectionGeometry()
    {
        for (const Connection &connection : connections_)
            updateConnection(connection.line, connection.from, connection.to);
    }

    void updateConnections()
    {
        updateConnectionGeometry();
        updateSceneBounds();
    }

signals:
    void nodeDoubleClicked(NodeItem *node);

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override
    {
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
        if (auto *node = dynamic_cast<NodeItem *>(item))
            emit nodeDoubleClicked(node);
        QGraphicsScene::mouseDoubleClickEvent(event);
    }

private:
    struct Connection {
        NodeItem *from;
        NodeItem *to;
        QGraphicsPathItem *line;
        QString type;
    };

    NodeItem *findNodeById(const char *id) const
    {
        for (NodeItem *node : nodes_) {
            if (std::strcmp(node->workflowNode()->id, id) == 0)
                return node;
        }
        return nullptr;
    }

    void addRelationshipLines(NodeItem *from, size_t count,
                              const char ids[][WORKFLOW_MAX_NAME], const QString &type)
    {
        for (size_t i = 0; i < count; ++i) {
            NodeItem *to = findNodeById(ids[i]);
            if (!to || to == from)
                continue;

            auto *line = new QGraphicsPathItem;
            QPen pen;
            if (type == "Simultaneous")
                pen = QPen(QColor(90, 190, 120), 2);
            else if (type == "Next Action")
                pen = QPen(QColor(230, 170, 70), 2);
            else
                pen = QPen(QColor(70, 160, 230), 2);
            line->setPen(pen);
            line->setZValue(-1);
            connections_.push_back({from, to, line, type});
            addItem(line);
        }
    }

    static void updateConnection(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
    {
        const QPointF a = from->sceneBoundingRect().center();
        const QPointF b = to->sceneBoundingRect().center();
        const qreal dx = (b.x() - a.x()) * 0.5;
        QPainterPath path(a);
        path.cubicTo(a + QPointF(dx, 0), b - QPointF(dx, 0), b);
        line->setPath(path);
    }

    void updateSceneBounds()
    {
        if (nodes_.isEmpty()) {
            setSceneRect(0, 0, 2000, 1400);
            return;
        }

        QRectF bounds;
        bool haveBounds = false;
        for (NodeItem *node : nodes_) {
            const QRectF nodeBounds = node->sceneBoundingRect();
            bounds = haveBounds ? bounds.united(nodeBounds) : nodeBounds;
            haveBounds = true;
        }
        if (!haveBounds || !bounds.isValid()) {
            setSceneRect(0, 0, 2000, 1400);
            return;
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

    int nextId_ = 0;
    QVector<NodeItem *> nodes_;
    QVector<Connection> connections_;
};

class WorkflowGraphicsView final : public QGraphicsView {
public:
    explicit WorkflowGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent)
    {
        setRenderHint(QPainter::Antialiasing);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        setDragMode(QGraphicsView::RubberBandDrag);
    }

    void zoomIn() { scale(1.15, 1.15); updateZoomLabel(); }
    void zoomOut() { scale(1.0 / 1.15, 1.0 / 1.15); updateZoomLabel(); }
    void resetZoom() { resetTransform(); updateZoomLabel(); }

    void fitAll()
    {
        if (!scene() || scene()->items().isEmpty()) {
            resetZoom();
            return;
        }
        const QRectF bounds = scene()->itemsBoundingRect().adjusted(-80, -80, 80, 80);
        if (bounds.isValid() && !bounds.isEmpty())
            fitInView(bounds, Qt::KeepAspectRatio);
        updateZoomLabel();
    }

    void setZoomLabel(QLabel *label) { zoomLabel_ = label; updateZoomLabel(); }

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (event->angleDelta().y() == 0) {
            QGraphicsView::wheelEvent(event);
            return;
        }
        const qreal factor = event->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15);
        scale(factor, factor);
        updateZoomLabel();
        event->accept();
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::MiddleButton) {
            panning_ = true;
            panStart_ = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        QGraphicsView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (panning_) {
            const QPoint delta = event->pos() - panStart_;
            panStart_ = event->pos();
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
            event->accept();
            return;
        }
        QGraphicsView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::MiddleButton && panning_) {
            panning_ = false;
            unsetCursor();
            event->accept();
            return;
        }
        QGraphicsView::mouseReleaseEvent(event);
    }

private:
    void updateZoomLabel()
    {
        if (zoomLabel_)
            zoomLabel_->setText(QString("%1%").arg(qRound(transform().m11() * 100.0)));
    }

    QLabel *zoomLabel_ = nullptr;
    bool panning_ = false;
    QPoint panStart_;
};

class NodeSettingsDialog final : public QDialog {
public:
    NodeSettingsDialog(NodeItem *node, const QList<NodeItem *> &nodes, QWidget *parent = nullptr)
        : QDialog(parent), node_(node), nodes_(nodes)
    {
        setWindowTitle(QString("Node Settings - %1").arg(node ? node->nodeName() : "Node"));
        resize(560, 800);
        setMinimumSize(500, 400);

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(8, 8, 8, 8);
        root->setSpacing(8);

        auto *contentArea = new QScrollArea(this);
        contentArea->setWidgetResizable(true);
        contentArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        contentArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        auto *contentWidget = new QWidget;
        auto *contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(2, 2, 2, 2);
        contentLayout->setSpacing(8);
        contentWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        auto *nameBox = new QGroupBox("Node", contentWidget);
        auto *nameLayout = new QVBoxLayout(nameBox);
        name_ = new QLineEdit(node ? node->nodeName() : QString(), nameBox);
        nameLayout->addWidget(new QLabel("Name", nameBox));
        nameLayout->addWidget(name_);
        contentLayout->addWidget(nameBox);

        auto *actionBox = new QGroupBox("Existing Move / Swap / Value Action", contentWidget);
        auto *actionLayout = new QVBoxLayout(actionBox);

        triggerAction_ = new QComboBox(actionBox);
        triggerAction_->addItem("None");
        triggerAction_->addItem("Frontend Action");
        triggerAction_->addItem("Source Visibility");
        triggerAction_->addItem("Source Mute");
        triggerAction_->addItem("Source Audio Track");
        triggerAction_->addItem("Source Hotkey");
        triggerAction_->addItem("Filter Enable");
        triggerAction_->addItem("Frontend Hotkey");
        triggerAction_->addItem("Setting");
        triggerAction_->addItem("UDP packet");
        triggerAction_->addItem("Websocket Request");
        triggerAction_->addItem("Websocket Event");

        source_ = new QComboBox(actionBox);
        source_->setEditable(false);
        filter_ = new QComboBox(actionBox);
        filter_->setEditable(false);

        if (node) {
            const int triggerIndex = triggerAction_->findText(read_text(node->workflowNode()->trigger.action));
            if (triggerIndex >= 0)
                triggerAction_->setCurrentIndex(triggerIndex);
        }

        addLabeled(actionLayout, "Action", triggerAction_);
        actionLayout->addWidget(new QLabel("For the first node only: this is the action that will trigger the workflow.", actionBox));
        addLabeled(actionLayout, "Source", source_);
        addLabeled(actionLayout, "Filter", filter_);
        contentLayout->addWidget(actionBox);

        populateSources(node ? read_text(node->workflowNode()->action.scene_name) : QString());
        connect(source_, &QComboBox::currentIndexChanged, this, [this] { populateFilters(); });
        populateFilters(node ? read_text(node->workflowNode()->action.filter_name) : QString());

        auto *durationBox = new QGroupBox("Duration", contentWidget);
        auto *durationLayout = new QHBoxLayout(durationBox);
        durationMode_ = makeModeCombo(durationBox);
        durationMs_ = new QSpinBox(durationBox);
        durationMs_->setRange(0, 3600000);
        durationMs_->setSuffix(" ms");
        if (node) {
            setMode(durationMode_, node->workflowNode()->duration.mode);
            durationMs_->setValue((int)node->workflowNode()->duration.duration_ms);
        }
        durationLayout->addWidget(durationMode_);
        durationLayout->addWidget(durationMs_);
        contentLayout->addWidget(durationBox);

        startDelayBox_ = new QGroupBox("Start Delay", contentWidget);
        auto *delayLayout = new QHBoxLayout(startDelayBox_);
        startDelayMode_ = makeModeCombo(startDelayBox_);
        startDelayMs_ = new QSpinBox(startDelayBox_);
        startDelayMs_->setRange(0, 3600000);
        startDelayMs_->setSuffix(" ms");
        if (node) {
            setMode(startDelayMode_, node->workflowNode()->start_delay.mode);
            startDelayMs_->setValue((int)node->workflowNode()->start_delay.delay_ms);
        }
        delayLayout->addWidget(startDelayMode_);
        delayLayout->addWidget(startDelayMs_);
        contentLayout->addWidget(startDelayBox_);
        updateDelayAvailability();

        auto *triggerBox = new QGroupBox("Start / Stop Triggers", contentWidget);
        auto *triggerLayout = new QGridLayout(triggerBox);
        startTriggerMode_ = makeModeCombo(triggerBox);
        stopTriggerMode_ = makeModeCombo(triggerBox);
        startTriggerValue_ = new QLineEdit(triggerBox);
        stopTriggerValue_ = new QLineEdit(triggerBox);
        triggerLayout->addWidget(new QLabel("Start Trigger", triggerBox), 0, 0);
        triggerLayout->addWidget(startTriggerMode_, 0, 1);
        triggerLayout->addWidget(startTriggerValue_, 0, 2);
        triggerLayout->addWidget(new QLabel("Stop Trigger", triggerBox), 1, 0);
        triggerLayout->addWidget(stopTriggerMode_, 1, 1);
        triggerLayout->addWidget(stopTriggerValue_, 1, 2);
        if (node) {
            setMode(startTriggerMode_, node->workflowNode()->start_trigger_mode);
            setMode(stopTriggerMode_, node->workflowNode()->stop_trigger_mode);
            startTriggerValue_->setText(read_text(node->workflowNode()->start_trigger_value));
            stopTriggerValue_->setText(read_text(node->workflowNode()->stop_trigger_value));
        }
        contentLayout->addWidget(triggerBox);

        simultaneous_ = makeNodeList(node, node ? node->workflowNode()->simultaneous_node_ids : nullptr,
                                     node ? node->workflowNode()->simultaneous_node_count : 0);
        endActions_ = makeNodeList(node, node ? node->workflowNode()->end_node_ids : nullptr,
                                   node ? node->workflowNode()->end_node_count : 0);
        nextActions_ = makeNodeList(node, node ? node->workflowNode()->next_node_ids : nullptr,
                                    node ? node->workflowNode()->next_node_count : 0);

        contentLayout->addWidget(makeListBox("Simultaneous Actions", simultaneous_,
                                    "Selected nodes start with this node. Each node keeps its own settings."));
        contentLayout->addWidget(makeListBox("End Actions", endActions_,
                                    "Selected nodes start when this node finishes. Each selected node's Start Delay applies."));
        contentLayout->addWidget(makeListBox("Next Actions", nextActions_,
                                    "Director chaining. This does not inherit the selected Move filter's own Next Action."));

        auto *nextBox = new QGroupBox("Next Move On", contentWidget);
        auto *nextLayout = new QGridLayout(nextBox);
        nextMoveOnMode_ = makeModeCombo(nextBox);
        nextMoveOnValue_ = new QComboBox(nextBox);
        nextMoveOnValue_->addItems({"Move End", "Move Start", "Move Start or End"});
        nextLayout->addWidget(new QLabel("Mode", nextBox), 0, 0);
        nextLayout->addWidget(nextMoveOnMode_, 0, 1);
        nextLayout->addWidget(new QLabel("Value", nextBox), 1, 0);
        nextLayout->addWidget(nextMoveOnValue_, 1, 1);
        if (node) {
            setMode(nextMoveOnMode_, node->workflowNode()->next_move_on_mode);
            const int index = nextMoveOnValue_->findText(read_text(node->workflowNode()->next_move_on_value));
            if (index >= 0)
                nextMoveOnValue_->setCurrentIndex(index);
        }
        contentLayout->addWidget(nextBox);
        contentLayout->addStretch(1);

        contentArea->setWidget(contentWidget);
        root->addWidget(contentArea, 1);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        connect(buttons, &QDialogButtonBox::accepted, this, [this] { if (apply()) accept(); });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        root->addWidget(buttons);
    }

    bool apply()
    {
        if (!node_)
            return false;

        workflow_node_t *wf = node_->workflowNode();
        const QString name = name_->text().trimmed();
        const QString parentSource = source_->currentData().toString();
        const QString filterName = filter_->currentData().toString();
        if (name.isEmpty() || parentSource.isEmpty() || filterName.isEmpty())
            return false;

        obs_source_t *parent = obs_get_source_by_name(parentSource.toUtf8().constData());
        if (!parent)
            return false;
        obs_source_t *filter = obs_source_get_filter_by_name(parent, filterName.toUtf8().constData());
        if (!filter) {
            obs_source_release(parent);
            return false;
        }

        const char *filterId = obs_source_get_id(filter);
        if (!is_supported_move_filter(filterId)) {
            obs_source_release(filter);
            obs_source_release(parent);
            return false;
        }

        copy_text(wf->name, WORKFLOW_MAX_NAME, name);
        copy_text(wf->trigger.action, WORKFLOW_MAX_NAME, triggerAction_->currentText());

        copy_text(wf->action.scene_name, WORKFLOW_MAX_NAME, parentSource);
        wf->action.source_name[0] = '\0';
        copy_text(wf->action.filter_name, WORKFLOW_MAX_NAME, filterName);
        copy_text(wf->action.filter_id, WORKFLOW_MAX_NAME, filterId);
        wf->action.kind = workflow_kind_from_filter_id(filterId);

        wf->duration.mode = (workflow_value_mode_t)durationMode_->currentData().toInt();
        wf->duration.duration_ms = (uint64_t)durationMs_->value();

        wf->start_delay.mode = (workflow_value_mode_t)startDelayMode_->currentData().toInt();
        wf->start_delay.delay_ms = (uint64_t)startDelayMs_->value();

        wf->start_trigger_mode = (workflow_value_mode_t)startTriggerMode_->currentData().toInt();
        wf->stop_trigger_mode = (workflow_value_mode_t)stopTriggerMode_->currentData().toInt();
        wf->next_move_on_mode = (workflow_value_mode_t)nextMoveOnMode_->currentData().toInt();
        copy_text(wf->start_trigger_value, WORKFLOW_MAX_VALUE, startTriggerValue_->text());
        copy_text(wf->stop_trigger_value, WORKFLOW_MAX_VALUE, stopTriggerValue_->text());
        copy_text(wf->next_move_on_value, WORKFLOW_MAX_VALUE, nextMoveOnValue_->currentText());

        wf->simultaneous_actions_mode = WORKFLOW_OVERRIDE;
        wf->end_actions_mode = WORKFLOW_OVERRIDE;
        wf->next_actions_mode = WORKFLOW_OVERRIDE;
        set_node_list(wf->simultaneous_node_count, wf->simultaneous_node_ids, simultaneous_);
        set_node_list(wf->end_node_count, wf->end_node_ids, endActions_);
        set_node_list(wf->next_node_count, wf->next_node_ids, nextActions_);

        obs_source_release(filter);
        obs_source_release(parent);
        return true;
    }

private:
    static void addLabeled(QLayout *layout, const QString &label, QWidget *widget)
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel(label));
        row->addWidget(widget, 1);
        layout->addItem(row);
    }

    static QComboBox *makeModeCombo(QWidget *parent)
    {
        auto *combo = new QComboBox(parent);
        combo->addItem("Use existing", WORKFLOW_USE_EXISTING);
        combo->addItem("Override", WORKFLOW_OVERRIDE);
        return combo;
    }

    static void setMode(QComboBox *combo, workflow_value_mode_t mode)
    {
        const int index = combo->findData((int)mode);
        if (index >= 0)
            combo->setCurrentIndex(index);
    }

    QWidget *makeListBox(const QString &title, QListWidget *list, const QString &hint)
    {
        auto *box = new QGroupBox(title, this);
        auto *layout = new QVBoxLayout(box);
        layout->addWidget(new QLabel(hint, box));
        layout->addWidget(list);
        return box;
    }

    QListWidget *makeNodeList(NodeItem *current, const char ids[][WORKFLOW_MAX_NAME], size_t count)
    {
        auto *list = new QListWidget(this);
        list->setSelectionMode(QAbstractItemView::MultiSelection);
        for (NodeItem *candidate : nodes_) {
            if (candidate == current)
                continue;
            auto *item = new QListWidgetItem(candidate->nodeName(), list);
            item->setData(Qt::UserRole, candidate->id());
            if (ids && list_contains(ids, count, candidate->id()))
                item->setSelected(true);
        }
        list->setMinimumHeight(110);
        list->setMaximumHeight(180);
        return list;
    }

    static bool addSourceToCombo(void *data, obs_source_t *source)
    {
        auto *combo = static_cast<QComboBox *>(data);
        if (!combo || !source)
            return true;
        const QString name = QString::fromUtf8(obs_source_get_name(source));
        if (combo->findData(name) < 0)
            combo->addItem(name, name);
        return true;
    }

    void populateSources(const QString &wanted)
    {
        source_->blockSignals(true);
        source_->clear();
        obs_enum_scenes(addSourceToCombo, source_);
        obs_enum_sources(addSourceToCombo, source_);
        source_->blockSignals(false);

        const int index = source_->findData(wanted);
        if (index >= 0)
            source_->setCurrentIndex(index);
        else if (source_->count() > 0)
            source_->setCurrentIndex(0);
    }

    static void addFilterToCombo(obs_source_t *parent, obs_source_t *filter, void *param)
    {
        Q_UNUSED(parent);
        auto *combo = static_cast<QComboBox *>(param);
        if (!combo || !filter)
            return;
        const char *id = obs_source_get_id(filter);
        if (!is_supported_move_filter(id))
            return;
        const QString name = QString::fromUtf8(obs_source_get_name(filter));
        combo->addItem(name, name);
    }

    void populateFilters(const QString &wanted = QString())
    {
        filter_->blockSignals(true);
        filter_->clear();
        const QString parentName = source_->currentData().toString();
        if (!parentName.isEmpty()) {
            obs_source_t *parent = obs_get_source_by_name(parentName.toUtf8().constData());
            if (parent) {
                obs_source_enum_filters(parent, addFilterToCombo, filter_);
                obs_source_release(parent);
            }
        }
        filter_->blockSignals(false);

        const int index = filter_->findData(wanted);
        if (index >= 0)
            filter_->setCurrentIndex(index);
        else if (filter_->count() > 0)
            filter_->setCurrentIndex(0);
        updateDelayAvailability();
    }

    void updateDelayAvailability()
    {
        if (!startDelayBox_)
            return;

        const QString parentName = source_->currentData().toString();
        const QString filterName = filter_->currentData().toString();
        bool relevant = false;
        if (!parentName.isEmpty() && !filterName.isEmpty()) {
            obs_source_t *parent = obs_get_source_by_name(parentName.toUtf8().constData());
            if (parent) {
                obs_source_t *filter = obs_source_get_filter_by_name(parent, filterName.toUtf8().constData());
                if (filter) {
                    const workflow_move_kind_t kind = workflow_kind_from_filter_id(obs_source_get_id(filter));
                    relevant = kind == WORKFLOW_MOVE_SOURCE || kind == WORKFLOW_MOVE_SWAP;
                    obs_source_release(filter);
                }
                obs_source_release(parent);
            }
        }
        startDelayBox_->setEnabled(relevant);
    }

    NodeItem *node_ = nullptr;
    QList<NodeItem *> nodes_;
    QLineEdit *name_ = nullptr;
    QComboBox *triggerAction_ = nullptr;
    QComboBox *source_ = nullptr;
    QComboBox *filter_ = nullptr;
    QComboBox *durationMode_ = nullptr;
    QSpinBox *durationMs_ = nullptr;
    QGroupBox *startDelayBox_ = nullptr;
    QComboBox *startDelayMode_ = nullptr;
    QSpinBox *startDelayMs_ = nullptr;
    QComboBox *startTriggerMode_ = nullptr;
    QComboBox *stopTriggerMode_ = nullptr;
    QComboBox *nextMoveOnMode_ = nullptr;
    QLineEdit *startTriggerValue_ = nullptr;
    QLineEdit *stopTriggerValue_ = nullptr;
    QComboBox *nextMoveOnValue_ = nullptr;
    QListWidget *simultaneous_ = nullptr;
    QListWidget *endActions_ = nullptr;
    QListWidget *nextActions_ = nullptr;
};

class EditorWindow final : public QDialog {
public:
    explicit EditorWindow(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Move Workflow Editor");
        resize(1050, 700);

        auto *root = new QVBoxLayout(this);
        auto *toolbar = new QHBoxLayout;
        auto *add = new QPushButton("+ Add Node", this);
        auto *edit = new QPushButton("Edit Node", this);
        auto *zoomOut = new QPushButton("−", this);
        auto *zoomReset = new QPushButton("100%", this);
        auto *zoomIn = new QPushButton("+", this);
        auto *fit = new QPushButton("Fit All", this);
        auto *close = new QPushButton("Close", this);
        toolbar->addWidget(add);
        toolbar->addWidget(edit);
        toolbar->addStretch();
        toolbar->addWidget(zoomOut);
        toolbar->addWidget(zoomReset);
        toolbar->addWidget(zoomIn);
        toolbar->addWidget(fit);
        toolbar->addWidget(close);
        root->addLayout(toolbar);

        auto *hint = new QLabel(
            "Add nodes, drag them around the canvas, then double-click a node to configure its existing action and Director settings. "
            "End, simultaneous and next relationships are selected directly in the node settings. Mouse wheel zooms; middle mouse pans.", this);
        hint->setWordWrap(true);
        root->addWidget(hint);

        scene_ = new EditorScene(this);
        view_ = new WorkflowGraphicsView(scene_, this);
        root->addWidget(view_, 1);

        auto *status = new QHBoxLayout;
        status->addStretch();
        status->addWidget(new QLabel("Zoom:", this));
        zoomLabel_ = new QLabel("100%", this);
        status->addWidget(zoomLabel_);
        root->addLayout(status);
        view_->setZoomLabel(zoomLabel_);

        connect(add, &QPushButton::clicked, this, [this] {
            bool ok = false;
            const QString name = QInputDialog::getText(this, "Add Node", "Node name:", QLineEdit::Normal,
                                                        "New Node", &ok);
            if (ok && !name.trimmed().isEmpty())
                scene_->addNode(name.trimmed());
        });
        connect(edit, &QPushButton::clicked, this, [this] { editSelectedNode(); });
        connect(zoomOut, &QPushButton::clicked, view_, &WorkflowGraphicsView::zoomOut);
        connect(zoomReset, &QPushButton::clicked, view_, &WorkflowGraphicsView::resetZoom);
        connect(zoomIn, &QPushButton::clicked, view_, &WorkflowGraphicsView::zoomIn);
        connect(fit, &QPushButton::clicked, view_, &WorkflowGraphicsView::fitAll);
        connect(close, &QPushButton::clicked, this, &QDialog::hide);
        connect(scene_, &QGraphicsScene::selectionChanged, this, [this] { scene_->updateConnections(); });
        connect(scene_, &EditorScene::nodeDoubleClicked, this, [this](NodeItem *node) { editNode(node); });
    }

private:
    void editNode(NodeItem *node)
    {
        if (!node)
            return;
        NodeSettingsDialog dialog(node, scene_->nodes(), this);
        if (dialog.exec() == QDialog::Accepted) {
            node->refreshDisplay();
            scene_->refreshConnectionsFor(node);
        }
    }

    void editSelectedNode() { editNode(scene_->selectedNode()); }

    EditorScene *scene_ = nullptr;
    WorkflowGraphicsView *view_ = nullptr;
    QLabel *zoomLabel_ = nullptr;
};

QPointer<EditorWindow> window;

void show_editor()
{
    if (!window) {
        auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        window = new EditorWindow(mainWindow);
    }
    window->show();
    window->raise();
    window->activateWindow();
}

void register_menu()
{
    QAction *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction("Move Workflow Editor"));
    if (!action)
        return;
    QObject::connect(action, &QAction::triggered, [] { show_editor(); });
}

struct AutoRegister {
    AutoRegister()
    {
        QTimer::singleShot(0, register_menu);
    }
};

AutoRegister auto_register;

} // namespace

#include "move-workflow-editor.moc"

void move_workflow_register_editor(void)
{
    QTimer::singleShot(0, register_menu);
}
