#include "workflow-action-list-ui.h"

#include "workflow-node.h"

#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <cstring>

namespace {
NodeItem *find_id(const QList<NodeItem *> &nodes, const QString &id)
{
    for (NodeItem *node : nodes)
        if (node && node->id().compare(id, Qt::CaseInsensitive) == 0)
            return node;
    return nullptr;
}
const workflow_shortcut_binding_t *find_binding(const workflow_node_t *source, const QString &target)
{
    if (!source)
        return nullptr;
    const QByteArray id = target.toUtf8();
    for (size_t i = 0; i < source->shortcut_binding_count; ++i)
        if (std::strcmp(source->shortcut_bindings[i].target_id, id.constData()) == 0)
            return &source->shortcut_bindings[i];
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

void workflow_action_list_rebuild_rows(
    QVBoxLayout *layout,
    const QList<NodeItem *> &nodes,
    const QStringList &attached_ids,
    const std::function<void(const QString &)> &remove_callback,
    bool shortcut_mode,
    QList<QKeySequenceEdit *> *shortcut_editors,
    const workflow_node_t *source)
{
    if (!layout)
        return;
    if (shortcut_editors)
        shortcut_editors->clear();
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
    const bool showIds = layout->parentWidget() &&
                         layout->parentWidget()->property("showNodeIds").toBool();
    for (const QString &id : attached_ids) {
        NodeItem *node = find_id(nodes, id);
        const QString display_name = showIds || !node ? id : node->nodeName();
        auto *row = new QWidget(layout->parentWidget());
        row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        auto *row_layout = new QVBoxLayout(row);
        row_layout->setContentsMargins(4, 2, 2, 2);
        auto *top = new QHBoxLayout;
        auto *label = new QLabel(display_name, row);
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        top->addWidget(label, 1);
        auto *remove_button = new QPushButton("Remove", row);
        remove_button->setStyleSheet(
            "QPushButton { min-width: 76px; max-width: 76px; padding: 3px 8px; "
            "text-align: center; }");
        remove_button->setToolTip("Remove this action");
        top->addWidget(remove_button, 0, Qt::AlignTop);
        row_layout->addLayout(top);
        if (shortcut_mode && node) {
            auto *edit = new QKeySequenceEdit(row);
            edit->setObjectName("shortcutKey_" + node->id());
            if (const auto *binding = find_binding(source, node->id()))
                edit->setKeySequence(QKeySequence::fromString(
                    QString::fromUtf8(binding->key), QKeySequence::PortableText));
            edit->setToolTip(QString("Shortcut for %1").arg(display_name));
            row_layout->addWidget(edit);
            if (shortcut_editors)
                shortcut_editors->append(edit);
        }
        layout->addWidget(row);
        QObject::connect(remove_button, &QPushButton::clicked, row,
                         [remove_callback, id] { remove_callback(id); });
    }
}
