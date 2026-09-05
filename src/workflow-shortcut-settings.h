#pragma once

#include "workflow-model.h"
#include <QList>
#include <QString>

class NodeItem;
class QWidget;

namespace workflow_shortcut_settings {

struct Binding {
    QString source_id;
    QString target_id;
    QString key;
};

QWidget *create_editor(const workflow_node_t *source,
                       const QList<NodeItem *> &nodes,
                       QWidget *parent = nullptr);

bool read(const QWidget *editor, Binding &binding);
bool apply(const Binding &binding, workflow_node_t *source);

} // namespace workflow_shortcut_settings
