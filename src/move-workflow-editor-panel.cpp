#include <QAbstractItemView>
#include <QApplication>
#include <QDialog>
#include <QFrame>
#include <QFont>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <algorithm>

namespace {

struct NodeEntry {
    QGraphicsRectItem *item = nullptr;
    QString name;
    QString type;
};

static QList<NodeEntry> findNodes(QGraphicsScene *scene)
{
    QList<NodeEntry> result;
    if (!scene)
        return result;

    for (QGraphicsItem *graphicsItem : scene->items(Qt::AscendingOrder)) {
        auto *rect = dynamic_cast<QGraphicsRectItem *>(graphicsItem);
        if (!rect || rect->parentItem())
            continue;

        QString name;
        QString type;
        for (QGraphicsItem *child : rect->childItems()) {
            auto *text = dynamic_cast<QGraphicsTextItem *>(child);
            if (!text)
                continue;
            const QString value = text->toPlainText().trimmed();
            if (value.isEmpty())
                continue;
            if (name.isEmpty())
                name = value;
            else if (type.isEmpty())
                type = value;
        }

        if (name.isEmpty() || (type != "TRIGGER" && type != "ACTION"))
            continue;

        result.push_back({rect, name, type});
    }

    std::sort(result.begin(), result.end(), [](const NodeEntry &a, const NodeEntry &b) {
        const QPointF ap = a.item->scenePos();
        const QPointF bp = b.item->scenePos();
        if (!qFuzzyCompare(ap.y() + 1.0, bp.y() + 1.0))
            return ap.y() < bp.y();
        return ap.x() < bp.x();
    });
    return result;
}

static QPushButton *findButton(QWidget *window, const QString &text)
{
    for (QPushButton *button : window->findChildren<QPushButton *>()) {
        if (button->text() == text)
            return button;
    }
    return nullptr;
}

class WorkflowPanelInstaller final : public QObject {
public:
    WorkflowPanelInstaller()
    {
        timer_.setInterval(400);
        connect(&timer_, &QTimer::timeout, this, &WorkflowPanelInstaller::installOnOpenEditors);
        timer_.start();
        QTimer::singleShot(0, this, &WorkflowPanelInstaller::installOnOpenEditors);
    }

private:
    void installOnOpenEditors()
    {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            auto *dialog = qobject_cast<QDialog *>(widget);
            if (!dialog || dialog->windowTitle() != "Move Workflow Editor")
                continue;
            if (dialog->property("moveWorkflowSidePanelInstalled").toBool())
                continue;
            install(dialog);
        }
    }

    void install(QDialog *dialog)
    {
        auto *view = dialog->findChild<QGraphicsView *>();
        if (!view || !view->scene() || !dialog->layout())
            return;

        auto *root = qobject_cast<QVBoxLayout *>(dialog->layout());
        if (!root)
            return;

        int viewIndex = -1;
        for (int i = 0; i < root->count(); ++i) {
            QLayoutItem *item = root->itemAt(i);
            if (item && item->widget() == view) {
                viewIndex = i;
                break;
            }
        }
        if (viewIndex < 0)
            return;

        auto *panel = new QFrame(dialog);
        panel->setObjectName("moveWorkflowSidePanel");
        panel->setFrameShape(QFrame::StyledPanel);
        panel->setMinimumWidth(245);
        panel->setMaximumWidth(285);
        panel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

        auto *panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(10, 10, 10, 10);
        panelLayout->setSpacing(8);

        auto *workflowTitle = new QLabel("WORKFLOW", panel);
        QFont titleFont = workflowTitle->font();
        titleFont.setBold(true);
        workflowTitle->setFont(titleFont);
        panelLayout->addWidget(workflowTitle);

        auto *workflowName = new QLabel("Move Workflow", panel);
        workflowName->setStyleSheet("color: #b8c0cc;");
        panelLayout->addWidget(workflowName);

        auto *separator = new QFrame(panel);
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        panelLayout->addWidget(separator);

        auto *nodesTitle = new QLabel("NODES", panel);
        QFont nodesFont = nodesTitle->font();
        nodesFont.setBold(true);
        nodesTitle->setFont(nodesFont);
        panelLayout->addWidget(nodesTitle);

        auto *nodeList = new QListWidget(panel);
        nodeList->setSelectionMode(QAbstractItemView::SingleSelection);
        nodeList->setMinimumHeight(180);
        nodeList->setAlternatingRowColors(false);
        panelLayout->addWidget(nodeList, 1);

        auto *addButton = new QPushButton("+ Add Node", panel);
        auto *editButton = new QPushButton("Edit Selected", panel);
        auto *deleteButton = new QPushButton("Delete Selected", panel);
        editButton->setEnabled(false);
        deleteButton->setEnabled(false);
        panelLayout->addWidget(addButton);
        panelLayout->addWidget(editButton);
        panelLayout->addWidget(deleteButton);

        auto *selectionTitle = new QLabel("SELECTION", panel);
        QFont selectionFont = selectionTitle->font();
        selectionFont.setBold(true);
        selectionTitle->setFont(selectionFont);
        panelLayout->addWidget(selectionTitle);

        auto *selectionLabel = new QLabel("No node selected", panel);
        selectionLabel->setWordWrap(true);
        selectionLabel->setStyleSheet("color: #b8c0cc;");
        panelLayout->addWidget(selectionLabel);

        auto *rootItem = root->takeAt(viewIndex);
        delete rootItem;

        auto *canvasRow = new QHBoxLayout;
        canvasRow->setSpacing(10);
        canvasRow->addWidget(panel, 0);
        canvasRow->addWidget(view, 1);
        root->insertLayout(viewIndex, canvasRow, 1);

        auto refreshList = [nodeList, selectionLabel, editButton, deleteButton, scene = view->scene()] {
            QGraphicsItem *selected = nullptr;
            for (QGraphicsItem *item : scene->selectedItems()) {
                if (dynamic_cast<QGraphicsRectItem *>(item) && !item->parentItem()) {
                    selected = item;
                    break;
                }
            }

            const QSignalBlocker blocker(nodeList);
            nodeList->clear();
            const QList<NodeEntry> nodes = findNodes(scene);
            int selectedRow = -1;
            for (const NodeEntry &node : nodes) {
                auto *item = new QListWidgetItem(QString("%1\n%2").arg(node.name, node.type), nodeList);
                item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(
                                               reinterpret_cast<quintptr>(node.item)));
                if (node.item == selected)
                    selectedRow = nodeList->count() - 1;
            }

            if (selectedRow >= 0)
                nodeList->setCurrentRow(selectedRow);

            const bool hasSelection = selected != nullptr;
            editButton->setEnabled(hasSelection);
            deleteButton->setEnabled(hasSelection);
            if (selected) {
                QString name;
                QString type;
                for (QGraphicsItem *child : selected->childItems()) {
                    if (auto *text = dynamic_cast<QGraphicsTextItem *>(child)) {
                        const QString value = text->toPlainText().trimmed();
                        if (name.isEmpty())
                            name = value;
                        else if (type.isEmpty())
                            type = value;
                    }
                }
                selectionLabel->setText(QString("%1\n%2").arg(name, type));
            } else {
                selectionLabel->setText("No node selected");
            }
        };

        auto selectCanvasItem = [nodeList, scene = view->scene()] {
            QListWidgetItem *item = nodeList->currentItem();
            if (!item)
                return;
            const quintptr value = static_cast<quintptr>(item->data(Qt::UserRole).toULongLong());
            auto *node = reinterpret_cast<QGraphicsItem *>(value);
            if (!node)
                return;
            scene->clearSelection();
            node->setSelected(true);
            scene->update();
        };

        QObject::connect(nodeList, &QListWidget::currentRowChanged, panel, [selectCanvasItem](int) {
            selectCanvasItem();
        });
        QObject::connect(nodeList, &QListWidget::itemDoubleClicked, panel,
                         [editButton](QListWidgetItem *) { editButton->click(); });

        QObject::connect(view->scene(), &QGraphicsScene::selectionChanged, panel, refreshList);
        QObject::connect(view->scene(), &QGraphicsScene::changed, panel, [refreshList] {
            QTimer::singleShot(0, refreshList);
        });

        if (auto *existingAdd = findButton(dialog, "+ Add Node"))
            QObject::connect(addButton, &QPushButton::clicked, existingAdd, &QPushButton::click);
        if (auto *existingEdit = findButton(dialog, "Edit Node"))
            QObject::connect(editButton, &QPushButton::clicked, existingEdit, &QPushButton::click);
        if (auto *existingDelete = findButton(dialog, "Delete Node"))
            QObject::connect(deleteButton, &QPushButton::clicked, existingDelete, &QPushButton::click);

        dialog->setProperty("moveWorkflowSidePanelInstalled", true);
        refreshList();
    }

    QTimer timer_;
};

WorkflowPanelInstaller installer;

} // namespace
