#pragma once

#include "workflow-node.h"
#include "workflow-model.h"

#include <QList>
#include <QString>
#include <QWidget>

class WorkflowShortcutList final : public QWidget {
public:
    WorkflowShortcutList(const QString &title,
                         const QString &hint,
                         NodeItem *current,
                         const QList<NodeItem *> &nodes,
                         const char ids[][WORKFLOW_MAX_NAME],
                         const uint32_t *keys,
                         const uint32_t *modifiers,
                         size_t count,
                         QWidget *parent = nullptr);

    void apply(size_t &count,
               char ids[][WORKFLOW_MAX_NAME],
               uint32_t *keys,
               uint32_t *modifiers) const;

private:
    struct Entry {
        QString nodeId;
        uint32_t key = OBS_KEY_NONE;
        uint32_t modifiers = 0;
    };

    void rebuildAttachedList();
    void addAction();
    void removeAction(const QString &id);

    NodeItem *current_ = nullptr;
    QList<NodeItem *> nodes_;
    QList<Entry> entries_;
    class QLineEdit *search_ = nullptr;
    class QVBoxLayout *attachedLayout_ = nullptr;
};
