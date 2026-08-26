#pragma once
#include "workflow-model.h"
#ifdef __cplusplus
extern "C" {
#endif
bool workflow_change_scene_execute(const workflow_node_t *node);
uint64_t workflow_change_scene_transition_duration(void);
#ifdef __cplusplus
}
#endif
