#pragma once

#include <QList>
#include <QString>
#include <QStringList>

class NodeItem;

namespace workflow_editor_node_order {

QStringList ordered_node_ids(const QString &workflowId,
                             const QList<NodeItem *> &nodes);
QStringList stored_order(const QString &workflowId);
void save_order(const QString &workflowId, const QStringList &nodeIds);
QStringList merge_order(const QStringList &storedIds,
                        const QList<NodeItem *> &nodes);

} // namespace workflow_editor_node_order
