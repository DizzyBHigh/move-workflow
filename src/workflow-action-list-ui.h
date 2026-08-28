#pragma once

#include <QList>
#include <QStringList>

class NodeItem;
class QVBoxLayout;

QStringList workflow_action_list_names(const QList<NodeItem *> &nodes,
                                       NodeItem *current);

NodeItem *workflow_action_list_find_match(const QList<NodeItem *> &nodes,
                                          NodeItem *current,
                                          const QString &query);

void workflow_action_list_rebuild_rows(QVBoxLayout *layout,
                                       const QList<NodeItem *> &nodes,
                                       const QStringList &attached_ids);
