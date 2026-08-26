#include "workflow-scene.h"
#include "workflow-node.h"
#include "workflow-node-settings-common.h"

EditorScene::EditorScene(QObject *parent) : QGraphicsScene(parent)
{
    connect(this, &QGraphicsScene::changed, this, [this] { updateConnections(); });
}

NodeItem *EditorScene::addNode(workflow_node_type_t type, const QString &name)
{
    EditorNode node;
    node.numeric_id = ++nextId_;
    copy_text(node.workflow.id, WORKFLOW_MAX_NAME, QString("node-%1").arg(node.numeric_id));
    copy_text(node.workflow.name, WORKFLOW_MAX_NAME, name);
    node.workflow.type = type;
    node.workflow.trigger_count = 0;
    node.workflow.duration.mode = WORKFLOW_OVERRIDE;
    node.workflow.start_delay.mode = WORKFLOW_OVERRIDE;
    node.workflow.end_delay.mode = WORKFLOW_OVERRIDE;
    node.workflow.simultaneous_actions_mode = WORKFLOW_OVERRIDE;
    node.workflow.next_actions_mode = WORKFLOW_OVERRIDE;
    node.position = QPointF(80 + ((node.numeric_id - 1) % 4) * 310,
                            80 + ((node.numeric_id - 1) / 4) * 190);

    auto *item = new NodeItem(node);
    addItem(item);
    return item;
}
