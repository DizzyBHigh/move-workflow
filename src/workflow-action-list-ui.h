#pragma once

#include <QList>
#include <QStringList>
#include <functional>

class NodeItem;
class QKeySequenceEdit;
class QVBoxLayout;
struct workflow_node_t;

QStringList workflow_action_list_names(const QList<NodeItem *> &nodes,
                                       NodeItem *current);

NodeItem *workflow_action_list_find_match(const QList<NodeItem *> &nodes,
                                          NodeItem *current,
                                          const QString &query);

void workflow_action_list_rebuild_rows(
    QVBoxLayout *layout,
    const QList<NodeItem *> &nodes,
    const QStringList &attached_ids,
    const std::function<void(const QString &)> &remove_callback,
    bool shortcut_mode = false,
    QList<QKeySequenceEdit *> *shortcut_editors = nullptr,
    const workflow_node_t *source = nullptr);
