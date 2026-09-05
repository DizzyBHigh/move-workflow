#include "workflow-shortcut-settings.h"
#include "workflow-node.h"

#include <QComboBox>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <cstdio>

namespace workflow_shortcut_settings {

static int binding_index(const workflow_node_t *source, const QString &target)
{
    if (!source) return -1;
    for (size_t i = 0; i < source->shortcut_binding_count; ++i)
        if (QString::fromUtf8(source->shortcut_bindings[i].target_id) == target) return (int)i;
    return -1;
}

QWidget *create_editor(const workflow_node_t *source, const QList<NodeItem *> &nodes, QWidget *parent)
{
    auto *box = new QWidget(parent); auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *target = new QComboBox(box);
    for (NodeItem *node : nodes) {
        if (!node || !node->workflowNode() || node->workflowNode() == source ||
            node->workflowNode()->type != WORKFLOW_NODE_ACTION) continue;
        if (source && binding_index(source, node->id()) < 0) continue;
        target->addItem(node->nodeName(), node->id());
    }
    layout->addWidget(new QLabel("Shortcut Action", box)); layout->addWidget(target);
    auto *key = new QKeySequenceEdit(box); key->setToolTip("Press the shortcut key combination.");
    layout->addWidget(new QLabel("Shortcut Key", box)); layout->addWidget(key);
    auto *clear = new QPushButton("Clear", box); layout->addWidget(clear);
    QObject::connect(clear, &QPushButton::clicked, key, &QKeySequenceEdit::clear);
    QObject::connect(target, &QComboBox::currentIndexChanged, [source, target, key](int) {
        if (!source) return;
        const int index = binding_index(source, target->currentData().toString());
        key->setKeySequence(index >= 0 ? QKeySequence::fromString(
            QString::fromUtf8(source->shortcut_bindings[index].key)) : QKeySequence());
    });
    if (target->count()) target->setCurrentIndex(0);
    return box;
}

bool read(const QWidget *editor, Binding &binding)
{
    if (!editor) return false;
    const auto *target = editor->findChild<QComboBox *>();
    const auto *key = editor->findChild<QKeySequenceEdit *>();
    if (!target || !key || target->currentIndex() < 0) return false;
    binding.target_id = target->currentData().toString();
    binding.key = key->keySequence().toString(QKeySequence::PortableText);
    return !binding.target_id.isEmpty();
}

bool apply(const Binding &binding, workflow_node_t *source)
{
    if (!source || binding.target_id.isEmpty()) return false;
    const int index = binding_index(source, binding.target_id);
    if (index < 0) return false;
    const QByteArray key = binding.key.toUtf8();
    std::snprintf(source->shortcut_bindings[index].key, WORKFLOW_MAX_NAME, "%s", key.constData());
    return true;
}

} // namespace workflow_shortcut_settings
