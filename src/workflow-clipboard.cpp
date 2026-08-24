#include "workflow-clipboard.h"

#include "workflow-scene.h"
#include "workflow-node.h"

#include <QGraphicsItem>
#include <QPointF>
#include <QList>
#include <cstring>
#include <vector>

namespace {
struct ClipNode {
    workflow_node_t node{};
    QPointF position;
};
std::vector<ClipNode> clipboard;

void remap(char ids[][WORKFLOW_MAX_NAME], size_t &count,
           const std::vector<QString> &oldIds, const std::vector<QString> &newIds)
{
    for (size_t i = 0; i < count; ++i) {
        const QString oldId = QString::fromUtf8(ids[i]);
        for (size_t j = 0; j < oldIds.size(); ++j) {
            if (oldId != oldIds[j])
                continue;
            const QByteArray value = newIds[j].toUtf8();
            std::strncpy(ids[i], value.constData(), WORKFLOW_MAX_NAME - 1);
            ids[i][WORKFLOW_MAX_NAME - 1] = '\0';
            break;
        }
    }
}
}

bool workflow_clipboard_copy(EditorScene *scene)
{
    clipboard.clear();
    if (!scene)
        return false;
    for (NodeItem *node : scene->nodes()) {
        if (!node || !node->isSelected())
            continue;
        ClipNode copy;
        copy.node = *node->workflowNode();
        copy.position = node->pos();
        clipboard.push_back(copy);
    }
    return !clipboard.empty();
}

bool workflow_clipboard_paste(EditorScene *scene)
{
    if (!scene || clipboard.empty())
        return false;

    std::vector<QString> oldIds;
    std::vector<QString> newIds;
    QList<NodeItem *> pasted;
    const QPointF offset(30.0, 30.0);
    scene->clearSelection();

    for (const ClipNode &clip : clipboard) {
        QString id;
        for (int n = 1; ; ++n) {
            id = QString("node_copy_%1").arg(n);
            bool used = false;
            for (NodeItem *node : scene->nodes())
                used |= node->id() == id;
            for (const QString &newId : newIds)
                used |= newId == id;
            if (!used)
                break;
        }
        NodeItem *node = scene->addNode(clip.node.type, QString::fromUtf8(clip.node.name));
        if (!node)
            return false;
        *node->workflowNode() = clip.node;
        const QByteArray idBytes = id.toUtf8();
        std::strncpy(node->workflowNode()->id, idBytes.constData(), WORKFLOW_MAX_NAME - 1);
        node->workflowNode()->id[WORKFLOW_MAX_NAME - 1] = '\0';
        node->setPos(clip.position + offset);
        oldIds.push_back(QString::fromUtf8(clip.node.id));
        newIds.push_back(id);
        pasted.push_back(node);
    }

    for (NodeItem *node : pasted) {
        workflow_node_t *data = node->workflowNode();
        remap(data->end_node_ids, data->end_node_count, oldIds, newIds);
        remap(data->simultaneous_node_ids, data->simultaneous_node_count, oldIds, newIds);
        remap(data->next_node_ids, data->next_node_count, oldIds, newIds);
        remap(data->shortcut_node_ids, data->shortcut_node_count, oldIds, newIds);
        node->setSelected(true);
    }
    scene->rebuildConnections();
    return true;
}

bool workflow_clipboard_has_data()
{
    return !clipboard.empty();
}
