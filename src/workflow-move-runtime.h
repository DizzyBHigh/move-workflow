#pragma once

#include <stdbool.h>

#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Executes a Move filter through Move Transition's public OBS hotkey path.
 * This avoids depending on Move Transition's private implementation symbols.
 */
bool workflow_move_runtime_trigger(workflow_t *workflow, workflow_node_t *node);

#ifdef __cplusplus
}
#endif
