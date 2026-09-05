#pragma once

#include "workflow-model.h"

#include <QString>

class QWidget;
class QVBoxLayout;

namespace workflow_shortcut_settings {

struct Binding {
    QString source_id;
    QString target_id;
    QString key;
};

QWidget *create_editor(const workflow_node_t *source,
                       const QList<class NodeItem *> &nodes,
                       QWidget *parent = nullptr);

bool apply(const Binding &binding, workflow_node_t *source);

} // namespace workflow_shortcut_settings
