#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct workflow_engine;
void workflow_engine_service_set(struct workflow_engine *engine);
bool workflow_engine_service_test_node(const char *workflow_id, const char *node_id);

#ifdef __cplusplus
}
#endif
