#pragma once

#include "workflow-node.h"

#include <QList>
#include <QString>
#include <QVector>
#include <QWidget>

#include <cstddef>
#include <cstdint>

class QLineEdit;
class QVBoxLayout;
class WorkflowShortcutKeyEdit;

class WorkflowShortcutList final : public QWidget {
public:
    WorkflowShortcutList(const QString &title,
                         const QString &hint,
                         NodeItem *current,
                         const QList<NodeItem *> &nodes,
                         const char ids[][WORKFLOW_MAX_NAME],
                         const uint32_t keys[],
                         const uint32_t modifiers[],
                         size_t count,
                         QWidget *parent = nullptr);

    void apply(size_t &count,
               char ids[][WORKFLOW_MAX_NAME],
               uint32_t keys[],
               uint32_t modifiers[]) const;

private:
    struct ShortcutRow {
        QString id;
        WorkflowShortcutKeyEdit *key = nullptr;
    };

    void rebuildAttachedList();
    void addAction();
    void removeAction(const QString &id);

    NodeItem *current_ = nullptr;
    QList<NodeItem *> nodes_;
    QVector<ShortcutRow> rows_;
    QLineEdit *search_ = nullptr;
    QVBoxLayout *attachedLayout_ = nullptr;
};
