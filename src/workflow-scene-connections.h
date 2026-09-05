#pragma once

#include "workflow-node.h"

#include <QList>
#include <QString>

namespace workflow_scene_connections {

QString relationship_type(const workflow_node_t *source,
                          const workflow_node_t *target);

bool add_relationship(workflow_node_t *source,
                      workflow_node_t *target,
                      const QString &type);

bool remove_relationship(workflow_node_t *source,
                         workflow_node_t *target,
                         const QString &type);

bool has_relationship(const workflow_node_t *source,
                      const char *targetId,
                      const QString &type);

workflow_node_t *find_node(const QList<NodeItem *> &nodes,
                           const char *id);

}
