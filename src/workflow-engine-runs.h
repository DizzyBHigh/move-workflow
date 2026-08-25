#pragma once

#include "workflow-engine-state.h"
#include "workflow-trigger-types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct workflow_engine_run workflow_engine_run_t;
typedef struct workflow_engine_runs workflow_engine_runs_t;

workflow_engine_runs_t *workflow_engine_runs_create(void);
void workflow_engine_runs_destroy(workflow_engine_runs_t *runs);
workflow_engine_run_t *workflow_engine_runs_start(workflow_engine_runs_t *runs,
                                                   workflow_t *workflow);
workflow_engine_state_t *workflow_engine_run_state(workflow_engine_run_t *run);
workflow_engine_run_t *workflow_engine_runs_current(workflow_engine_runs_t *runs);
void workflow_engine_runs_stop_all(workflow_engine_runs_t *runs);
bool workflow_engine_runs_any_active(const workflow_engine_runs_t *runs);
bool workflow_engine_runs_trigger(workflow_engine_runs_t *runs,
                                   workflow_trigger_type_t type,
                                   const char *value);

#ifdef __cplusplus
}
#endif
