#include "move-workflow-editor.h"

#include <obs-frontend-api.h>

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QTimer>
#include <QVector>
#include <QPointer>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QScrollBar>

#include <memory>

namespace {

struct EditorNode {
    int id = 0;
    QString name;
    QPointF position;
};

class NodeItem final : public QGraphicsRectItem {
public:
    NodeItem(EditorNode node, QGraphicsItem *parent = nullptr)
        : QGraphicsRectItem(parent), node_(std::move(node))
    {
        setRect(0, 0, 220, 90);
        setBrush(QColor(42, 45, 50));
        setPen(QPen(QColor(110, 120, 135), 1));
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges);
        setPos(node_.position);

        title_ = new QGraphicsTextItem(node_.name, this);
        title_->setDefaultTextColor(Qt::white);
        title_->setPos(12, 10);

        details_ = new QGraphicsTextItem("Existing Move / Swap / Value action", this);
        details_->setDefaultTextColor(QColor(185, 190, 200));
        details_->setPos(12, 38);
    }

    int nodeId() const { return node_.id; }
    const QString &nodeName() const { return node_.name; }
    void setNodeName(const QString &name)
    {
        node_.name = name;
        title_->setPlainText(name);
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
        connect(this, &QGraphicsScene::changed, this, [this] { updateConnections(); });
    }

    NodeItem *addNode(const QString &name)
    {
        EditorNode node;
        node.id = ++nextId_;
        node.name = name;
        node.position = QPointF(80 + (node.id % 4) * 250, 80 + ((node.id - 1) / 4) * 140);
        auto *item = new NodeItem(node);
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

    void connectNodes(NodeItem *from, NodeItem *to)
    {
        if (!from || !to || from == to)
            return;

        auto *line = new QGraphicsPathItem;
        line->setPen(QPen(QColor(70, 160, 230), 2));
        line->setZValue(-1);
        connections_.push_back({from, to, line});
        addItem(line);
        updateConnection(line, from, to);
        updateSceneBounds();
    }

    void updateConnections()
    {
        for (const Connection &connection : connections_)
            updateConnection(connection.line, connection.from, connection.to);

        // Keep the scene bounds tied to the actual node positions. This lets a
        // node be dragged beyond the previous edge and automatically expands
        // the canvas instead of leaving the node outside the scrollable area.
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
    };

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
            if (!node)
                continue;

            const QRectF nodeBounds = node->sceneBoundingRect();
            if (!haveBounds) {
                bounds = nodeBounds;
                haveBounds = true;
            } else {
                bounds = bounds.united(nodeBounds);
            }
        }

        if (!haveBounds || !bounds.isValid()) {
            setSceneRect(0, 0, 2000, 1400);
            return;
        }

        // Give the editor a comfortable working margin around the outermost
        // nodes. The margin is part of the scene, so nodes can sit at any edge
        // without becoming clipped or inaccessible.
        constexpr qreal margin = 160.0;
        bounds.adjust(-margin, -margin, margin, margin);

        // Keep a sensible minimum canvas size when only one or two nodes exist.
        constexpr qreal minimumWidth = 900.0;
        constexpr qreal minimumHeight = 600.0;
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

    void zoomIn()
    {
        scale(1.15, 1.15);
        updateZoomLabel();
    }

    void zoomOut()
    {
        scale(1.0 / 1.15, 1.0 / 1.15);
        updateZoomLabel();
    }

    void resetZoom()
    {
        resetTransform();
        updateZoomLabel();
    }

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

    void setZoomLabel(QLabel *label)
    {
        zoomLabel_ = label;
        updateZoomLabel();
    }

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
        if (!zoomLabel_)
            return;
        const qreal zoom = transform().m11() * 100.0;
        zoomLabel_->setText(QString("%1%").arg(qRound(zoom)));
    }

    QLabel *zoomLabel_ = nullptr;
    bool panning_ = false;
    QPoint panStart_;
};

class NodeSettingsDialog final : public QDialog {
public:
    explicit NodeSettingsDialog(NodeItem *node, QWidget *parent = nullptr) : QDialog(parent), node_(node)
    {
        setWindowTitle("Node Settings");
        setMinimumWidth(430);

        auto *layout = new QVBoxLayout(this);
        auto *nameLabel = new QLabel("Node name", this);
        name_ = new QLineEdit(node ? node->nodeName() : QString(), this);
        layout->addWidget(nameLabel);
        layout->addWidget(name_);

        auto *action = new QLabel(
            "Existing action\nSelect the Move, Swap or Value action this Director node will hook into.\n\n"
            "Director overrides will be added to this panel in the next editor stage.", this);
        action->setWordWrap(true);
        layout->addWidget(action);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }

    bool apply()
    {
        if (!node_)
            return false;
        node_->setNodeName(name_->text().trimmed());
        return true;
    }

private:
    NodeItem *node_;
    QLineEdit *name_ = nullptr;
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
        auto *link = new QPushButton("Link Selected Nodes", this);
        auto *zoomOut = new QPushButton("−", this);
        auto *zoomReset = new QPushButton("100%", this);
        auto *zoomIn = new QPushButton("+", this);
        auto *fit = new QPushButton("Fit All", this);
        auto *close = new QPushButton("Close", this);

        toolbar->addWidget(add);
        toolbar->addWidget(edit);
        toolbar->addWidget(link);
        toolbar->addStretch();
        toolbar->addWidget(zoomOut);
        toolbar->addWidget(zoomReset);
        toolbar->addWidget(zoomIn);
        toolbar->addWidget(fit);
        toolbar->addWidget(close);
        root->addLayout(toolbar);

        auto *hint = new QLabel(
            "Add nodes, drag them around the canvas, double-click a node for settings, then select two nodes and link them. "
            "Use the mouse wheel or the zoom controls to navigate larger workflows. Hold the middle mouse button to pan the canvas.", this);
        hint->setWordWrap(true);
        root->addWidget(hint);

        scene_ = new EditorScene(this);
        scene_->setSceneRect(0, 0, 2000, 1400);
        view_ = new WorkflowGraphicsView(scene_, this);
        root->addWidget(view_, 1);

        auto *status = new QHBoxLayout;
        status->addStretch();
        auto *zoomText = new QLabel("Zoom:", this);
        zoomLabel_ = new QLabel("100%", this);
        status->addWidget(zoomText);
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
        connect(link, &QPushButton::clicked, this, [this] { linkSelectedNodes(); });
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
        NodeSettingsDialog dialog(node, this);
        if (dialog.exec() == QDialog::Accepted)
            dialog.apply();
    }

    void editSelectedNode()
    {
        editNode(scene_->selectedNode());
    }

    void linkSelectedNodes()
    {
        const auto selected = scene_->selectedItems();
        if (selected.size() != 2)
            return;
        auto *from = dynamic_cast<NodeItem *>(selected.at(0));
        auto *to = dynamic_cast<NodeItem *>(selected.at(1));
        scene_->connectNodes(from, to);
    }

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
