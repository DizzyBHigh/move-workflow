#include "move-workflow-editor.h"
#include "workflow-model.h"

#include <obs-frontend-api.h>
#include <obs.h>

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
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>
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
        setRect(0, 0, 270, 142);
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges);
        setPos(node_.position);
        refreshStyle();

        title_ = new QGraphicsTextItem(read_text(node_.workflow.name), this);
        title_->setDefaultTextColor(Qt::white);
        title_->setPos(14, 10);

        type_ = new QGraphicsTextItem(this);
        type_->setDefaultTextColor(QColor(175, 185, 200));
        type_->setPos(14, 34);

        details_ = new QGraphicsTextItem(this);
        details_->setDefaultTextColor(QColor(210, 215, 220));
        details_->setPos(14, 56);
        refreshDisplay();
    }

    QString id() const { return read_text(node_.workflow.id); }
    QString nodeName() const { return read_text(node_.workflow.name); }
    workflow_node_t *workflowNode() { return &node_.workflow; }
    const workflow_node_t *workflowNode() const { return &node_.workflow; }

    void refreshDisplay()
    {
        if (!title_ || !type_ || !details_)
            return;

        title_->setPlainText(read_text(node_.workflow.name));
        type_->setPlainText(node_.workflow.type == WORKFLOW_NODE_TRIGGER ? "TRIGGER" : "ACTION");

        if (node_.workflow.type == WORKFLOW_NODE_TRIGGER) {
            const QString trigger = read_text(node_.workflow.trigger.action);
            details_->setPlainText(QString("Trigger: %1\nConnections: %2")
                                       .arg(trigger.isEmpty() ? "None" : trigger)
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
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if (change == ItemPositionHasChanged)
            node_.position = value.toPointF();
        return QGraphicsRectItem::itemChange(change, value);
    }

private:
    void refreshStyle()
    {
        if (node_.workflow.type == WORKFLOW_NODE_TRIGGER) {
            setBrush(QColor(35, 70, 85));
            setPen(QPen(QColor(70, 180, 220), 2));
        } else {
            setBrush(QColor(42, 45, 50));
            setPen(QPen(QColor(110, 120, 135), 1));
        }
    }

    EditorNode node_;
    QGraphicsTextItem *title_ = nullptr;
    QGraphicsTextItem *type_ = nullptr;
    QGraphicsTextItem *details_ = nullptr;
};

class EditorScene final : public QGraphicsScene {
    Q_OBJECT
public:
    explicit EditorScene(QObject *parent = nullptr) : QGraphicsScene(parent)
    {
        connect(this, &QGraphicsScene::changed, this, [this] { updateConnectionGeometry(); });
    }

    NodeItem *addNode(workflow_node_type_t type, const QString &name)
    {
        EditorNode node;
        node.numeric_id = ++nextId_;
        copy_text(node.workflow.id, WORKFLOW_MAX_NAME, QString("node-%1").arg(node.numeric_id));
        copy_text(node.workflow.name, WORKFLOW_MAX_NAME, name);
        node.workflow.type = type;
        copy_text(node.workflow.trigger.action, WORKFLOW_MAX_NAME, "None");
        node.workflow.duration.mode = WORKFLOW_OVERRIDE;
        node.workflow.start_delay.mode = WORKFLOW_OVERRIDE;
        node.workflow.end_delay.mode = WORKFLOW_OVERRIDE;
        node.workflow.simultaneous_actions_mode = WORKFLOW_OVERRIDE;
        node.workflow.end_actions_mode = WORKFLOW_OVERRIDE;
        node.workflow.next_actions_mode = WORKFLOW_OVERRIDE;
        node.position = QPointF(80 + ((node.numeric_id - 1) % 4) * 310,
                                80 + ((node.numeric_id - 1) / 4) * 190);

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

    void deleteNode(NodeItem *node)
    {
        if (!node)
            return;
        const QString id = node->id();
        for (NodeItem *candidate : nodes_) {
            workflow_node_t *wf = candidate->workflowNode();
            remove_id(wf->end_node_count, wf->end_node_ids, id);
            remove_id(wf->simultaneous_node_count, wf->simultaneous_node_ids, id);
            remove_id(wf->next_node_count, wf->next_node_ids, id);
        }
        nodes_.removeAll(node);
        removeItem(node);
        delete node;
        rebuildConnections();
        updateSceneBounds();
    }

    void refreshConnectionsFor(NodeItem *)
    {
        rebuildConnections();
        updateSceneBounds();
    }

    void rebuildConnections()
    {
        for (const Connection &connection : std::as_const(connections_)) {
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

    void updateConnections()
    {
        for (const Connection &connection : std::as_const(connections_))
            updateConnection(connection.line, connection.from, connection.to);
        updateSceneBounds();
    }

    void updateSceneBounds()
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

signals:
    void nodeDoubleClicked(NodeItem *node);

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (auto *node = dynamic_cast<NodeItem *>(itemAt(event->scenePos(), QTransform())))
            emit nodeDoubleClicked(node);
        QGraphicsScene::mouseDoubleClickEvent(event);
    }

private:
    struct Connection {
        NodeItem *from = nullptr;
        NodeItem *to = nullptr;
        QGraphicsPathItem *line = nullptr;
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
            if (type == "Simultaneous")
                line->setPen(QPen(QColor(90, 190, 120), 2));
            else if (type == "Next Action")
                line->setPen(QPen(QColor(230, 170, 70), 2));
            else
                line->setPen(QPen(QColor(70, 160, 230), 2));
            line->setZValue(-1);
            addItem(line);
            connections_.push_back({from, to, line, type});
            updateConnection(line, from, to);
        }
    }

    static void updateConnection(QGraphicsPathItem *line, NodeItem *from, NodeItem *to)
    {
        const QRectF aRect = from->sceneBoundingRect();
        const QRectF bRect = to->sceneBoundingRect();
        const QPointF a = QPointF(aRect.right(), aRect.center().y());
        const QPointF b = QPointF(bRect.left(), bRect.center().y());
        const qreal dx = qMax<qreal>(40.0, qAbs(b.x() - a.x()) * 0.45);
        QPainterPath path(a);
        path.cubicTo(a + QPointF(dx, 0), b - QPointF(dx, 0), b);
        line->setPath(path);
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
        const int delta = event->angleDelta().y();
        if (delta == 0) {
            QGraphicsView::wheelEvent(event);
            return;
        }
        const qreal factor = delta > 0 ? 1.15 : (1.0 / 1.15);
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

        auto *contentArea = new QScrollArea(this);
        contentArea->setWidgetResizable(true);
        contentArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        contentArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        auto *content = new QWidget;
        auto *layout = new QVBoxLayout(content);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(8);

        auto *nodeBox = new QGroupBox("Node", content);
        auto *nodeLayout = new QVBoxLayout(nodeBox);
        name_ = new QLineEdit(node ? node->nodeName() : QString(), nodeBox);
        nodeLayout->addWidget(new QLabel("Name", nodeBox));
        nodeLayout->addWidget(name_);
        layout->addWidget(nodeBox);

        if (node && node->workflowNode()->type == WORKFLOW_NODE_TRIGGER)
            buildTriggerEditor(nodeBox, nodeLayout);
        else
            buildActionEditor(content, layout);

        layout->addStretch(1);
        contentArea->setWidget(content);
        root->addWidget(contentArea, 1);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, [this] {
            if (apply())
                accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        root->addWidget(buttons);
    }

    bool apply()
    {
        if (!node_)
            return false;
        const QString name = name_->text().trimmed();
        if (name.isEmpty())
            return false;

        workflow_node_t *wf = node_->workflowNode();
        copy_text(wf->name, WORKFLOW_MAX_NAME, name);

        if (wf->type == WORKFLOW_NODE_TRIGGER) {
            copy_text(wf->trigger.action, WORKFLOW_MAX_NAME, triggerAction_->currentText());
            return true;
        }

        const QString parentName = source_->currentData().toString();
        const QString filterName = filter_->currentData().toString();
        if (parentName.isEmpty() || filterName.isEmpty())
            return false;

        obs_source_t *parent = obs_get_source_by_name(parentName.toUtf8().constData());
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

        copy_text(wf->action.scene_name, WORKFLOW_MAX_NAME, parentName);
        wf->action.source_name[0] = '\0';
        copy_text(wf->action.filter_name, WORKFLOW_MAX_NAME, filterName);
        copy_text(wf->action.filter_id, WORKFLOW_MAX_NAME, filterId);
        wf->action.kind = workflow_kind_from_filter_id(filterId);

        wf->start_delay.mode = WORKFLOW_OVERRIDE;
        wf->start_delay.delay_ms = (uint64_t)startDelayMs_->value();
        wf->duration.mode = WORKFLOW_OVERRIDE;
        wf->duration.duration_ms = (uint64_t)durationMs_->value();
        wf->end_delay.mode = WORKFLOW_OVERRIDE;
        wf->end_delay.delay_ms = (uint64_t)endDelayMs_->value();

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
    void buildTriggerEditor(QWidget *parent, QVBoxLayout *layout)
    {
        auto *box = new QGroupBox("Trigger", parent);
        auto *boxLayout = new QVBoxLayout(box);
        triggerAction_ = new QComboBox(box);
        triggerAction_->addItems({"None", "Frontend Action", "Source Visibility", "Source Mute",
                                  "Source Audio Track", "Source Hotkey", "Filter Enable",
                                  "Frontend Hotkey", "Setting", "UDP packet", "Websocket Request",
                                  "Websocket Event"});
        const int index = triggerAction_->findText(read_text(node_->workflowNode()->trigger.action));
        if (index >= 0)
            triggerAction_->setCurrentIndex(index);
        boxLayout->addWidget(new QLabel("Trigger Action", box));
        boxLayout->addWidget(triggerAction_);
        boxLayout->addWidget(new QLabel("A Trigger Node can start a workflow branch from anywhere in the graph.", box));
        layout->addWidget(box);
    }

    void buildActionEditor(QWidget *parent, QVBoxLayout *layout)
    {
        auto *targetBox = new QGroupBox("Existing Move / Swap / Value Action", parent);
        auto *targetLayout = new QVBoxLayout(targetBox);
        source_ = new QComboBox(targetBox);
        filter_ = new QComboBox(targetBox);
        targetLayout->addWidget(new QLabel("Source", targetBox));
        targetLayout->addWidget(source_);
        targetLayout->addWidget(new QLabel("Filter", targetBox));
        targetLayout->addWidget(filter_);
        layout->addWidget(targetBox);

        populateSources(read_text(node_->workflowNode()->action.scene_name));
        connect(source_, &QComboBox::currentIndexChanged, this, [this] { populateFilters(); });
        populateFilters(read_text(node_->workflowNode()->action.filter_name));

        auto *timingBox = new QGroupBox("Timing", parent);
        auto *timingLayout = new QVBoxLayout(timingBox);
        startDelayMs_ = makeMilliseconds(timingBox, "Start Delay");
        durationMs_ = makeMilliseconds(timingBox, "Duration");
        endDelayMs_ = makeMilliseconds(timingBox, "End Delay");
        if (node_) {
            startDelayMs_->setValue((int)node_->workflowNode()->start_delay.delay_ms);
            durationMs_->setValue((int)node_->workflowNode()->duration.duration_ms);
            endDelayMs_->setValue((int)node_->workflowNode()->end_delay.delay_ms);
        }
        addSpinRow(timingLayout, "Start Delay", startDelayMs_);
        addSpinRow(timingLayout, "Duration", durationMs_);
        addSpinRow(timingLayout, "End Delay", endDelayMs_);
        layout->addWidget(timingBox);

        simultaneous_ = makeNodeList(node_, node_->workflowNode()->simultaneous_node_ids,
                                     node_->workflowNode()->simultaneous_node_count);
        endActions_ = makeNodeList(node_, node_->workflowNode()->end_node_ids,
                                   node_->workflowNode()->end_node_count);
        nextActions_ = makeNodeList(node_, node_->workflowNode()->next_node_ids,
                                    node_->workflowNode()->next_node_count);

        layout->addWidget(makeListBox("Simultaneous Actions", simultaneous_,
                                      "These nodes start together with this Action."));
        layout->addWidget(makeListBox("End Actions", endActions_,
                                      "These nodes start after this Action completes and its End Delay has elapsed."));
        layout->addWidget(makeListBox("Next Actions", nextActions_,
                                      "These are the next workflow nodes after this Action."));
    }

    static QSpinBox *makeMilliseconds(QWidget *parent, const QString &)
    {
        auto *spin = new QSpinBox(parent);
        spin->setRange(0, 3600000);
        spin->setSuffix(" ms");
        return spin;
    }

    static void addSpinRow(QVBoxLayout *layout, const QString &label, QSpinBox *spin)
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel(label));
        row->addWidget(spin, 1);
        layout->addLayout(row);
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
        list->setMinimumHeight(90);
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
    }

    NodeItem *node_ = nullptr;
    QList<NodeItem *> nodes_;
    QLineEdit *name_ = nullptr;
    QComboBox *triggerAction_ = nullptr;
    QComboBox *source_ = nullptr;
    QComboBox *filter_ = nullptr;
    QSpinBox *startDelayMs_ = nullptr;
    QSpinBox *durationMs_ = nullptr;
    QSpinBox *endDelayMs_ = nullptr;
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
        addButton_ = new QPushButton("+ Add Node", this);
        auto *edit = new QPushButton("Edit Node", this);
        deleteButton_ = new QPushButton("Delete Node", this);
        auto *zoomOut = new QPushButton("−", this);
        auto *zoomReset = new QPushButton("100%", this);
        auto *zoomIn = new QPushButton("+", this);
        auto *fit = new QPushButton("Fit All", this);
        auto *close = new QPushButton("Close", this);
        toolbar->addWidget(addButton_);
        toolbar->addWidget(edit);
        toolbar->addWidget(deleteButton_);
        toolbar->addStretch();
        toolbar->addWidget(zoomOut);
        toolbar->addWidget(zoomReset);
        toolbar->addWidget(zoomIn);
        toolbar->addWidget(fit);
        toolbar->addWidget(close);
        root->addLayout(toolbar);

        auto *hint = new QLabel(
            "Trigger nodes start workflow branches. Action nodes reference an existing Move / Swap / Value filter. "
            "Drag nodes, double-click to edit, use the mouse wheel to zoom and middle mouse to pan.", this);
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

        connect(addButton_, &QPushButton::clicked, this, [this] { showAddNodeMenu(); });
        connect(edit, &QPushButton::clicked, this, [this] { editSelectedNode(); });
        connect(deleteButton_, &QPushButton::clicked, this, [this] { deleteSelectedNode(); });
        connect(zoomOut, &QPushButton::clicked, view_, &WorkflowGraphicsView::zoomOut);
        connect(zoomReset, &QPushButton::clicked, view_, &WorkflowGraphicsView::resetZoom);
        connect(zoomIn, &QPushButton::clicked, view_, &WorkflowGraphicsView::zoomIn);
        connect(fit, &QPushButton::clicked, view_, &WorkflowGraphicsView::fitAll);
        connect(close, &QPushButton::clicked, this, &QDialog::hide);
        connect(scene_, &QGraphicsScene::selectionChanged, this, [this] { updateButtonState(); });
        connect(scene_, &EditorScene::nodeDoubleClicked, this, [this](NodeItem *node) { editNode(node); });
        updateButtonState();
    }

private:
    void showAddNodeMenu()
    {
        QMenu menu(this);
        QAction *trigger = menu.addAction("Add Trigger Node");
        QAction *action = menu.addAction("Add Action Node");
        QAction *chosen = menu.exec(addButton_->mapToGlobal(QPoint(0, addButton_->height())));
        if (!chosen)
            return;

        bool ok = false;
        const QString name = QInputDialog::getText(this,
                                                    chosen == trigger ? "Add Trigger Node" : "Add Action Node",
                                                    "Node name:", QLineEdit::Normal,
                                                    chosen == trigger ? "New Trigger" : "New Action", &ok);
        if (!ok || name.trimmed().isEmpty())
            return;

        scene_->addNode(chosen == trigger ? WORKFLOW_NODE_TRIGGER : WORKFLOW_NODE_ACTION, name.trimmed());
    }

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

    void deleteSelectedNode()
    {
        NodeItem *node = scene_->selectedNode();
        if (!node)
            return;
        const QString name = node->nodeName();
        if (QMessageBox::question(this, "Delete Node",
                                  QString("Delete '%1'?\n\nAny connections to this node will also be removed.").arg(name),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
        scene_->deleteNode(node);
        updateButtonState();
    }

    void updateButtonState()
    {
        const bool selected = scene_ && scene_->selectedNode();
        deleteButton_->setEnabled(selected);
    }

    EditorScene *scene_ = nullptr;
    WorkflowGraphicsView *view_ = nullptr;
    QLabel *zoomLabel_ = nullptr;
    QPushButton *addButton_ = nullptr;
    QPushButton *deleteButton_ = nullptr;
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
    AutoRegister() { QTimer::singleShot(0, register_menu); }
};

AutoRegister auto_register;

} // namespace

#include "move-workflow-editor.moc"

void move_workflow_register_editor(void)
{
    QTimer::singleShot(0, register_menu);
}
