#include "workflow-shortcut-settings.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace workflow_shortcut_settings {

static QString node_name(const NodeItem *node)
{
    return node ? node->name() : QString();
}

QWidget *create_editor(const workflow_node_t *source,
                       const QList<NodeItem *> &nodes,
                       QWidget *parent)
{
    auto *box = new QWidget(parent);
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *target = new QComboBox(box);
    for (NodeItem *node : nodes) {
        if (!node || !node->workflowNode() || node->workflowNode() == source ||
            node->workflowNode()->type != WORKFLOW_NODE_ACTION)
            continue;
        target->addItem(node_name(node), node->id());
    }
    layout->addWidget(new QLabel("Shortcut Action", box));
    layout->addWidget(target);

    auto *key = new QLineEdit(box);
    key->setPlaceholderText("Press a key combination...");
    key->setReadOnly(true);
    layout->addWidget(new QLabel("Shortcut Key", box));
    layout->addWidget(key);

    auto *clear = new QPushButton("Clear", box);
    layout->addWidget(clear);
    QObject::connect(clear, &QPushButton::clicked, key, &QLineEdit::clear);
    return box;
}

bool apply(const Binding &binding, workflow_node_t *source)
{
    if (!source || binding.target_id.isEmpty() || binding.key.isEmpty())
        return false;
    return true;
}

} // namespace workflow_shortcut_settings
