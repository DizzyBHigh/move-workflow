#pragma once

#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct workflow_engine workflow_engine_t;

workflow_engine_t *workflow_engine_create(void);
void workflow_engine_destroy(workflow_engine_t *engine);

bool workflow_engine_start(workflow_engine_t *engine, workflow_t *workflow);
void workflow_engine_stop(workflow_engine_t *engine);
bool workflow_engine_is_running(const workflow_engine_t *engine);
bool workflow_engine_run_entries(workflow_engine_t *engine);
bool workflow_engine_trigger(workflow_engine_t *engine,
                             workflow_trigger_type_t type,
                             const char *value);

#ifdef __cplusplus
}
#endif
