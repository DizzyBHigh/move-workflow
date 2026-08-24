#include "workflow-action-list.h"

#include <QCompleter>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringListModel>
#include <QVBoxLayout>

#include <cstring>

namespace {

static void copy_text(char *destination, size_t capacity, const QString &value)
{
    if (!destination || capacity == 0)
        return;
    const QByteArray bytes = value.toUtf8();
    std::strncpy(destination, bytes.constData(), capacity - 1);
    destination[capacity - 1] = '\0';
}

static bool contains_id(const QStringList &ids, const QString &id)
{
    for (const QString &existing : ids) {
        if (existing.compare(id, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

} // namespace

WorkflowActionList::WorkflowActionList(const QString &title,
                                       const QString &hint,
                                       NodeItem *current,
                                       const QList<NodeItem *> &nodes,
                                       const char ids[][WORKFLOW_MAX_NAME],
                                       size_t count,
                                       QWidget *parent)
    : QWidget(parent), current_(current), nodes_(nodes)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(5);

    auto *titleLabel = new QLabel(title, this);
    titleLabel->setStyleSheet("font-weight: 600;");
    root->addWidget(titleLabel);

    auto *hintLabel = new QLabel(hint, this);
    hintLabel->setWordWrap(true);
    root->addWidget(hintLabel);

    auto *searchRow = new QHBoxLayout;
    search_ = new QLineEdit(this);
    search_->setPlaceholderText("Search actions...");
    searchRow->addWidget(search_, 1);
    auto *addButton = new QPushButton("Add", this);
    addButton->setToolTip("Add the matching action to this relationship");
    searchRow->addWidget(addButton);
    root->addLayout(searchRow);

    auto *completerModel = new QStringListModel(this);
    QStringList actionNames;
    for (NodeItem *node : nodes_) {
        if (!node || node == current_ || node->workflowNode()->type != WORKFLOW_NODE_ACTION)
            continue;
        actionNames.append(node->nodeName());
    }
    actionNames.sort(Qt::CaseInsensitive);
    completerModel->setStringList(actionNames);
    auto *completer = new QCompleter(completerModel, search_);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    search_->setCompleter(completer);

    connect(addButton, &QPushButton::clicked, this, [this] { addAction(); });
    connect(search_, &QLineEdit::returnPressed, this, [this] { addAction(); });

    auto *attachedBox = new QWidget(this);
    attachedLayout_ = new QVBoxLayout(attachedBox);
    attachedLayout_->setContentsMargins(0, 0, 0, 0);
    attachedLayout_->setSpacing(3);
    root->addWidget(attachedBox);

    for (size_t i = 0; i < count; ++i) {
        const QString id = QString::fromUtf8(ids[i]);
        if (!id.isEmpty() && !contains_id(attachedIds_, id))
            attachedIds_.append(id);
    }
    rebuildAttachedList();
}

void WorkflowActionList::apply(size_t &count, char ids[][WORKFLOW_MAX_NAME]) const
{
    count = 0;
    for (const QString &id : attachedIds_) {
        if (count >= WORKFLOW_MAX_LINKS)
            break;
        copy_text(ids[count], WORKFLOW_MAX_NAME, id);
        ++count;
    }
}

void WorkflowActionList::rebuildAttachedList()
{
    if (!attachedLayout_)
        return;

    while (QLayoutItem *item = attachedLayout_->takeAt(0)) {
        if (QWidget *widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    if (attachedIds_.isEmpty()) {
        auto *empty = new QLabel("No actions attached.", this);
        empty->setStyleSheet("color: #888;");
        attachedLayout_->addWidget(empty);
        return;
    }

    for (const QString &id : attachedIds_) {
        NodeItem *node = nullptr;
        for (NodeItem *candidate : nodes_) {
            if (candidate && candidate->id().compare(id, Qt::CaseInsensitive) == 0) {
                node = candidate;
                break;
            }
        }

        const QString displayName = node ? node->nodeName() : id;
        auto *row = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 2, 2, 2);
        auto *label = new QLabel(displayName, row);
        rowLayout->addWidget(label, 1);
        auto *removeButton = new QPushButton("X", row);
        removeButton->setFixedWidth(28);
        removeButton->setToolTip("Remove this action");
        rowLayout->addWidget(removeButton);
        attachedLayout_->addWidget(row);

        connect(removeButton, &QPushButton::clicked, this, [this, id] { removeAction(id); });
    }
}

void WorkflowActionList::addAction()
{
    if (!search_)
        return;

    const QString query = search_->text().trimmed();
    if (query.isEmpty())
        return;

    NodeItem *match = nullptr;
    for (NodeItem *node : nodes_) {
        if (!node || node == current_ || node->workflowNode()->type != WORKFLOW_NODE_ACTION)
            continue;
        if (node->nodeName().compare(query, Qt::CaseInsensitive) == 0 ||
            node->id().compare(query, Qt::CaseInsensitive) == 0) {
            match = node;
            break;
        }
    }

    if (!match) {
        for (NodeItem *node : nodes_) {
            if (!node || node == current_ || node->workflowNode()->type != WORKFLOW_NODE_ACTION)
                continue;
            if (node->nodeName().contains(query, Qt::CaseInsensitive)) {
                match = node;
                break;
            }
        }
    }

    if (!match || contains_id(attachedIds_, match->id()))
        return;

    if (attachedIds_.size() >= (int)WORKFLOW_MAX_LINKS)
        return;

    attachedIds_.append(match->id());
    search_->clear();
    rebuildAttachedList();
}

void WorkflowActionList::removeAction(const QString &id)
{
    for (int i = attachedIds_.size() - 1; i >= 0; --i) {
        if (attachedIds_.at(i).compare(id, Qt::CaseInsensitive) == 0)
            attachedIds_.removeAt(i);
    }
    rebuildAttachedList();
}
