#include "workflow-shortcut-list.h"
#include "workflow-shortcut-key-edit.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <cstdio>

WorkflowShortcutList::WorkflowShortcutList(const QString &title,
                                           const QString &hint,
                                           NodeItem *current,
                                           const QList<NodeItem *> &nodes,
                                           const char ids[][WORKFLOW_MAX_NAME],
                                           const uint32_t keys[],
                                           const uint32_t modifiers[],
                                           size_t count,
                                           QWidget *parent)
    : QWidget(parent), current_(current), nodes_(nodes)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("<b>%1</b>").arg(title), this));
    layout->addWidget(new QLabel(hint, this));
    search_ = new QLineEdit(this);
    search_->setPlaceholderText("Search actions...");
    layout->addWidget(search_);
    attachedLayout_ = new QVBoxLayout;
    layout->addLayout(attachedLayout_);

    for (size_t i = 0; i < count; ++i) {
        ShortcutRow row;
        row.id = QString::fromUtf8(ids[i]);
        row.key = new WorkflowShortcutKeyEdit(this);
        row.key->setCombination({modifiers[i], static_cast<obs_key_t>(keys[i])});
        rows_.append(row);
    }
    rebuildAttachedList();
    connect(search_, &QLineEdit::textChanged, this, [this] { rebuildAttachedList(); });
}

void WorkflowShortcutList::apply(size_t &count,
                                 char ids[][WORKFLOW_MAX_NAME],
                                 uint32_t keys[],
                                 uint32_t modifiers[]) const
{
    count = 0;
    for (const ShortcutRow &row : rows_) {
        if (count >= WORKFLOW_MAX_LINKS)
            break;
        QByteArray id = row.id.toUtf8();
        std::snprintf(ids[count], WORKFLOW_MAX_NAME, "%s", id.constData());
        const obs_key_combination_t combo = row.key->combination();
        keys[count] = static_cast<uint32_t>(combo.key);
        modifiers[count] = combo.modifiers;
        ++count;
    }
}

void WorkflowShortcutList::rebuildAttachedList()
{
    while (QLayoutItem *item = attachedLayout_->takeAt(0))
        delete item;

    const QString query = search_->text().trimmed();
    for (const ShortcutRow &row : rows_) {
        NodeItem *target = nullptr;
        for (NodeItem *node : nodes_) {
            if (node->workflowNode()->uuid == row.id)
                target = node;
        }
        const QString name = target ? target->nodeName() : row.id;
        if (!query.isEmpty() && !name.contains(query, Qt::CaseInsensitive))
            continue;
        auto *line = new QHBoxLayout;
        line->addWidget(new QLabel(name, this), 1);
        line->addWidget(row.key);
        auto *remove = new QPushButton("Remove", this);
        line->addWidget(remove);
        attachedLayout_->addLayout(line);
        connect(remove, &QPushButton::clicked, this, [this, id = row.id] { removeAction(id); });
    }
}

void WorkflowShortcutList::addAction() {}

void WorkflowShortcutList::removeAction(const QString &id)
{
    for (int i = 0; i < rows_.size(); ++i) {
        if (rows_[i].id == id) {
            rows_[i].key->deleteLater();
            rows_.removeAt(i);
            rebuildAttachedList();
            return;
        }
    }
}
