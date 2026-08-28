#include "move-workflow-editor-panel-ui.hpp"

#include <QAbstractItemView>
#include <QDialog>
#include <QFont>
#include <QFrame>
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
#include <algorithm>

namespace move_workflow_editor_panel_ui {
namespace {
struct NodeEntry { QGraphicsRectItem *item; QString name; QString type; };
QList<NodeEntry> findNodes(QGraphicsScene *scene)
{
    QList<NodeEntry> result; if (!scene) return result;
    for (QGraphicsItem *item : scene->items(Qt::AscendingOrder)) {
        auto *rect = dynamic_cast<QGraphicsRectItem *>(item);
        if (!rect || rect->parentItem()) continue;
        QString name, type;
        for (QGraphicsItem *child : rect->childItems()) {
            auto *text = dynamic_cast<QGraphicsTextItem *>(child); if (!text) continue;
            const QString value = text->toPlainText().trimmed(); if (value.isEmpty()) continue;
            if (name.isEmpty()) name = value; else if (type.isEmpty()) type = value;
        }
        if (!name.isEmpty() && (type == "TRIGGER" || type == "ACTION")) result.push_back({rect, name, type});
    }
    std::sort(result.begin(), result.end(), [](const NodeEntry &a, const NodeEntry &b) {
        const QPointF ap = a.item->scenePos(), bp = b.item->scenePos();
        if (!qFuzzyCompare(ap.y() + 1.0, bp.y() + 1.0)) return ap.y() < bp.y(); return ap.x() < bp.x();
    });
    return result;
}
QPushButton *findButton(QWidget *window, const QString &text)
{
    for (QPushButton *button : window->findChildren<QPushButton *>()) if (button->text() == text) return button;
    return nullptr;
}
QStringList nodeText(QGraphicsItem *node)
{
    QStringList values;
    for (QGraphicsItem *child : node->childItems()) if (auto *text = dynamic_cast<QGraphicsTextItem *>(child)) {
        const QString value = text->toPlainText().trimmed(); if (!value.isEmpty()) values.push_back(value);
    }
    return values;
}
}

void install(QDialog *dialog)
{
    auto *view = dialog->findChild<QGraphicsView *>(); auto *root = qobject_cast<QVBoxLayout *>(dialog->layout());
    if (!view || !view->scene() || !root) return;
    int viewIndex = -1;
    for (int i = 0; i < root->count(); ++i) if (auto *item = root->itemAt(i); item && item->widget() == view) { viewIndex = i; break; }
    if (viewIndex < 0) return;

    auto *panel = new QFrame(dialog); panel->setObjectName("moveWorkflowSidePanel"); panel->setFrameShape(QFrame::StyledPanel);
    panel->setMinimumWidth(245); panel->setMaximumWidth(285); panel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto *layout = new QVBoxLayout(panel); layout->setContentsMargins(10, 10, 10, 10); layout->setSpacing(8);
    auto *title = new QLabel("WORKFLOW", panel); auto font = title->font(); font.setBold(true); title->setFont(font); layout->addWidget(title);
    auto *name = new QLabel("Move Workflow", panel); name->setStyleSheet("color: #b8c0cc;"); layout->addWidget(name);
    auto *separator = new QFrame(panel); separator->setFrameShape(QFrame::HLine); separator->setFrameShadow(QFrame::Sunken); layout->addWidget(separator);
    auto *nodesTitle = new QLabel("NODES", panel); auto nodesFont = nodesTitle->font(); nodesFont.setBold(true); nodesTitle->setFont(nodesFont); layout->addWidget(nodesTitle);
    auto *nodeList = new QListWidget(panel); nodeList->setSelectionMode(QAbstractItemView::SingleSelection); nodeList->setMinimumHeight(180); layout->addWidget(nodeList, 1);
    auto *addButton = new QPushButton("+ Add Node", panel); auto *editButton = new QPushButton("Edit Selected", panel); auto *deleteButton = new QPushButton("Delete Selected", panel);
    editButton->setEnabled(false); deleteButton->setEnabled(false); layout->addWidget(addButton); layout->addWidget(editButton); layout->addWidget(deleteButton);
    auto *selectionTitle = new QLabel("SELECTION", panel); auto selectionFont = selectionTitle->font(); selectionFont.setBold(true); selectionTitle->setFont(selectionFont); layout->addWidget(selectionTitle);
    auto *selectionLabel = new QLabel("No node selected", panel); selectionLabel->setWordWrap(true); selectionLabel->setStyleSheet("color: #b8c0cc;"); layout->addWidget(selectionLabel);
    delete root->takeAt(viewIndex); auto *canvasRow = new QHBoxLayout; canvasRow->setSpacing(10); canvasRow->addWidget(panel, 0); canvasRow->addWidget(view, 1); root->insertLayout(viewIndex, canvasRow, 1);

    auto refreshList = [nodeList, selectionLabel, editButton, deleteButton, scene = view->scene()] {
        QGraphicsItem *selected = nullptr;
        for (QGraphicsItem *item : scene->selectedItems()) if (dynamic_cast<QGraphicsRectItem *>(item) && !item->parentItem()) { selected = item; break; }
        const QSignalBlocker blocker(nodeList); nodeList->clear(); int selectedRow = -1;
        for (const auto &node : findNodes(scene)) {
            auto *item = new QListWidgetItem(QString("%1\n%2").arg(node.name, node.type), nodeList);
            item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(reinterpret_cast<quintptr>(node.item)));
            if (node.item == selected) selectedRow = nodeList->count() - 1;
        }
        if (selectedRow >= 0) nodeList->setCurrentRow(selectedRow); editButton->setEnabled(selected); deleteButton->setEnabled(selected);
        if (selected) { const auto values = nodeText(selected); selectionLabel->setText(values.value(0) + "\n" + values.value(1)); } else selectionLabel->setText("No node selected");
    };
    QObject::connect(nodeList, &QListWidget::currentRowChanged, panel, [nodeList, scene = view->scene()](int) {
        auto *item = nodeList->currentItem(); if (!item) return;
        auto *node = reinterpret_cast<QGraphicsItem *>(static_cast<quintptr>(item->data(Qt::UserRole).toULongLong())); if (!node) return;
        scene->clearSelection(); node->setSelected(true); scene->update();
    });
    QObject::connect(nodeList, &QListWidget::itemDoubleClicked, panel, [editButton](QListWidgetItem *) { editButton->click(); });
    QObject::connect(view->scene(), &QGraphicsScene::selectionChanged, panel, refreshList);
    QObject::connect(view->scene(), &QGraphicsScene::changed, panel, [refreshList] { QTimer::singleShot(0, refreshList); });
    if (auto *button = findButton(dialog, "+ Add Node")) QObject::connect(addButton, &QPushButton::clicked, button, &QPushButton::click);
    if (auto *button = findButton(dialog, "Edit Node")) QObject::connect(editButton, &QPushButton::clicked, button, &QPushButton::click);
    if (auto *button = findButton(dialog, "Delete Node")) QObject::connect(deleteButton, &QPushButton::clicked, button, &QPushButton::click);
    dialog->setProperty("moveWorkflowSidePanelInstalled", true); refreshList();
}
}
