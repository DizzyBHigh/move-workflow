#pragma once

#include <QString>
#include <cstddef>

#include "workflow-model.h"

namespace workflow_scene_utils {

void copy_text(char *destination, size_t capacity, const QString &value);
bool add_node_id(size_t &count, char ids[][WORKFLOW_MAX_NAME], const QString &id);
void remove_id(size_t &count, char ids[][WORKFLOW_MAX_NAME], const QString &id);

} // namespace workflow_scene_utils
