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
#include <QMouseEvent>

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
    explicit EditorScene(QObject *parent = nullptr) : QGraphicsScene(parent) {}

    NodeItem *addNode(const QString &name)
    {
        EditorNode node;
        node.id = ++nextId_;
        node.name = name;
        node.position = QPointF(80 + (node.id % 4) * 250, 80 + ((node.id - 1) / 4) * 140);
        auto *item = new NodeItem(node);
        addItem(item);
        nodes_.push_back(item);
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
    }

    void updateConnections()
    {
        for (const Connection &connection : connections_)
            updateConnection(connection.line, connection.from, connection.to);
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

    int nextId_ = 0;
    QVector<NodeItem *> nodes_;
    QVector<Connection> connections_;
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
        auto *close = new QPushButton("Close", this);
        toolbar->addWidget(add);
        toolbar->addWidget(edit);
        toolbar->addWidget(link);
        toolbar->addStretch();
        toolbar->addWidget(close);
        root->addLayout(toolbar);

        auto *hint = new QLabel(
            "Add nodes, drag them around the canvas, double-click a node for settings, then select two nodes and link them.", this);
        hint->setWordWrap(true);
        root->addWidget(hint);

        scene_ = new EditorScene(this);
        scene_->setSceneRect(0, 0, 2000, 1400);
        view_ = new QGraphicsView(scene_, this);
        view_->setRenderHint(QPainter::Antialiasing);
        view_->setDragMode(QGraphicsView::RubberBandDrag);
        root->addWidget(view_, 1);

        connect(add, &QPushButton::clicked, this, [this] {
            bool ok = false;
            const QString name = QInputDialog::getText(this, "Add Node", "Node name:", QLineEdit::Normal,
                                                        "New Node", &ok);
            if (ok && !name.trimmed().isEmpty())
                scene_->addNode(name.trimmed());
        });

        connect(edit, &QPushButton::clicked, this, [this] { editSelectedNode(); });
        connect(link, &QPushButton::clicked, this, [this] { linkSelectedNodes(); });
        connect(close, &QPushButton::clicked, this, &QDialog::close);
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
    QGraphicsView *view_ = nullptr;
};

QPointer<EditorWindow> window;

void show_editor()
{
    if (!window) {
        auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        window = new EditorWindow(mainWindow);
        window->setAttribute(Qt::WA_DeleteOnClose);
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
