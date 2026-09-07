#pragma once
#include "workflow-engine-node-runtime.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct workflow_engine;
void workflow_engine_service_set(struct workflow_engine *engine);
bool workflow_engine_service_test_node(const char *workflow_id, const char *node_id);
bool workflow_engine_service_run_from_node(const char *workflow_id, const char *node_id);
bool workflow_engine_service_trigger(const char *workflow_id, const char *trigger_id);
bool workflow_engine_service_trigger_scene(const char *scene_name);
bool workflow_engine_service_workflow_running(const char *workflow_id);
bool workflow_engine_service_stop_workflow(const char *workflow_id);
bool workflow_engine_service_stop_run(const char *workflow_id, uint64_t run_id);
bool workflow_engine_service_accept_shortcut(const char *workflow_id, const char *source_id,
                                             const char *target_id);
bool workflow_engine_service_node_runtime(const char *workflow_id, const char *node_id,
                                          workflow_engine_node_runtime_t *out);
#ifdef __cplusplus
}
#endif
