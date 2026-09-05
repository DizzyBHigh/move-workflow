#pragma once

#include "workflow-node.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

class NodeItem;
class QVBoxLayout;
class QKeySequenceEdit;

class WorkflowActionList final : public QWidget {
public:
    WorkflowActionList(const QString &title,
                       const QString &hint,
                       NodeItem *current,
                       const QList<NodeItem *> &nodes,
                       const char ids[][WORKFLOW_MAX_NAME],
                       size_t count,
                       QWidget *parent = nullptr,
                       bool shortcut_mode = false);

    void apply(size_t &count, char ids[][WORKFLOW_MAX_NAME]) const;

private:
    void rebuildAttachedList();
    void addAction();
    void removeAction(const QString &id);
    void applyShortcutKeys() const;

    NodeItem *current_ = nullptr;
    QList<NodeItem *> nodes_;
    QStringList attachedIds_;
    QList<QKeySequenceEdit *> shortcutEditors_;
    class QLineEdit *search_ = nullptr;
    class QVBoxLayout *attachedLayout_ = nullptr;
    bool shortcutMode_ = false;
};
