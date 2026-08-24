#pragma once

#include "workflow-node.h"

#include <QList>
#include <QString>
#include <QWidget>

class WorkflowActionList final : public QWidget {
public:
    WorkflowActionList(const QString &title,
                       const QString &hint,
                       NodeItem *current,
                       const QList<NodeItem *> &nodes,
                       const char ids[][WORKFLOW_MAX_NAME],
                       size_t count,
                       QWidget *parent = nullptr);

    void apply(size_t &count, char ids[][WORKFLOW_MAX_NAME]) const;

private:
    void rebuildAttachedList();
    void addAction();
    void removeAction(const QString &id);

    NodeItem *current_ = nullptr;
    QList<NodeItem *> nodes_;
    QStringList attachedIds_;
    class QLineEdit *search_ = nullptr;
    class QVBoxLayout *attachedLayout_ = nullptr;
};
