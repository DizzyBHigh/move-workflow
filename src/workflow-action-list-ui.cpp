#include "workflow-action-list-ui.h"

#include "workflow-node.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
bool contains_id(const QStringList &ids, const QString &id)
{
    for (const QString &existing : ids)
        if (existing.compare(id, Qt::CaseInsensitive) == 0)
            return true;
    return false;
}

NodeItem *find_id(const QList<NodeItem *> &nodes, const QString &id)
{
    for (NodeItem *node : nodes)
        if (node && node->id().compare(id, Qt::CaseInsensitive) == 0)
            return node;
    return nullptr;
}
}

QStringList workflow_action_list_names(const QList<NodeItem *> &nodes, NodeItem *current)
{
    QStringList names;
    for (NodeItem *node : nodes) {
        if (!node || node == current || node->workflowNode()->type != WORKFLOW_NODE_ACTION)
            continue;
        names.append(node->nodeName());
    }
    names.sort(Qt::CaseInsensitive);
    return names;
}

NodeItem *workflow_action_list_find_match(const QList<NodeItem *> &nodes,
                                          NodeItem *current,
                                          const QString &query)
{
    for (NodeItem *node : nodes) {
        if (!node || node == current || node->workflowNode()->type != WORKFLOW_NODE_ACTION)
            continue;
        if (node->nodeName().compare(query, Qt::CaseInsensitive) == 0 ||
            node->id().compare(query, Qt::CaseInsensitive) == 0)
            return node;
    }
    for (NodeItem *node : nodes) {
        if (!node || node == current || node->workflowNode()->type != WORKFLOW_NODE_ACTION)
            continue;
        if (node->nodeName().contains(query, Qt::CaseInsensitive))
            return node;
    }
    return nullptr;
}

void workflow_action_list_rebuild_rows(QVBoxLayout *layout,
                                       const QList<NodeItem *> &nodes,
                                       const QStringList &attached_ids)
{
    if (!layout)
        return;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget())
            widget->deleteLater();
        delete item;
    }
    if (attached_ids.isEmpty()) {
        auto *empty = new QLabel("No actions attached.", layout->parentWidget());
        empty->setStyleSheet("color: #888;");
        layout->addWidget(empty);
        return;
    }
    for (const QString &id : attached_ids) {
        NodeItem *node = find_id(nodes, id);
        const QString display_name = node ? node->nodeName() : id;
        auto *row = new QWidget(layout->parentWidget());
        auto *row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(4, 2, 2, 2);
        auto *label = new QLabel(display_name, row);
        row_layout->addWidget(label, 1);
        auto *remove_button = new QPushButton("Remove", row);
        remove_button->setStyleSheet(
            "QPushButton { min-width: 76px; max-width: 76px; padding: 3px 8px; "
            "text-align: center; }");
        remove_button->setToolTip("Remove this action");
        row_layout->addWidget(remove_button);
        layout->addWidget(row);
    }
}
