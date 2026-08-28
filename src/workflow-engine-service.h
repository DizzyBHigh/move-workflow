#pragma once
#include "workflow-engine-node-runtime.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
struct workflow_engine;
void workflow_engine_service_set(struct workflow_engine *engine);
bool workflow_engine_service_test_node(const char *workflow_id, const char *node_id);
bool workflow_engine_service_trigger(const char *workflow_id, const char *trigger_id);
bool workflow_engine_service_trigger_scene(const char *scene_name);
bool workflow_engine_service_workflow_running(const char *workflow_id);
bool workflow_engine_service_node_runtime(const char *workflow_id, const char *node_id,
                                          workflow_engine_node_runtime_t *out);
#ifdef __cplusplus
}
#endif
