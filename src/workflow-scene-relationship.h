#pragma once

#include "workflow-model.h"

#include <QString>

namespace workflow_scene_relationship {

bool add(workflow_node_t *source, workflow_node_t *target,
         const QString &type);

bool remove(workflow_node_t *source, workflow_node_t *target,
            const QString &type);

bool has(const workflow_node_t *source, const char *target_id,
         const QString &type);

QString type_between(const workflow_node_t *source,
                     const workflow_node_t *target);

}
