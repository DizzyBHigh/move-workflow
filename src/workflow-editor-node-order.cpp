#include "workflow-editor-node-order.hpp"

#include "workflow-node.h"

#include <QSettings>

namespace workflow_editor_node_order {
namespace {
QString key(const QString &workflowId)
{
    return QStringLiteral("editor/node_order/") + workflowId;
}
}

QStringList merge_order(const QStringList &storedIds,
                        const QList<NodeItem *> &nodes)
{
    QStringList result;
    for (const QString &id : storedIds) {
        for (NodeItem *node : nodes) {
            if (node && node->id() == id) {
                result.append(id);
                break;
            }
        }
    }
    for (NodeItem *node : nodes) {
        if (!node || result.contains(node->id()))
            continue;
        result.append(node->id());
    }
    return result;
}

QStringList ordered_node_ids(const QString &workflowId,
                             const QList<NodeItem *> &nodes)
{
    if (workflowId.isEmpty()) {
        QStringList ids;
        for (NodeItem *node : nodes)
            if (node) ids.append(node->id());
        return ids;
    }
    QSettings settings;
    const QStringList stored = settings.value(key(workflowId)).toStringList();
    return merge_order(stored, nodes);
}

void save_order(const QString &workflowId, const QStringList &nodeIds)
{
    if (workflowId.isEmpty())
        return;
    QSettings settings;
    settings.setValue(key(workflowId), nodeIds);
    settings.sync();
}

} // namespace workflow_editor_node_order
