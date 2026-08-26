#include "workflow-engine-runs.h"
#include "workflow-debug.h"
#include "workflow-engine-trigger.h"
#include <cstdlib>

struct workflow_engine_run { workflow_engine_state_t state; workflow_engine_run_t *next; };
struct workflow_engine_runs { workflow_engine_run_t *head; workflow_engine_run_t *current; };
workflow_engine_runs_t *workflow_engine_runs_create(void) { return (workflow_engine_runs_t *)calloc(1, sizeof(workflow_engine_runs_t)); }
void workflow_engine_runs_destroy(workflow_engine_runs_t *runs) { if (!runs) return; for (auto *run=runs->head; run;) { auto *next=run->next; free(run); run=next; } free(runs); }
workflow_engine_run_t *workflow_engine_runs_start(workflow_engine_runs_t *runs, workflow_t *workflow) { if (!runs || !workflow || !workflow->enabled) return nullptr; auto *run=(workflow_engine_run_t *)calloc(1,sizeof(workflow_engine_run_t)); if(!run) return nullptr; workflow_engine_state_begin(&run->state,workflow); run->next=runs->head; runs->head=run; runs->current=run; workflow_debug_log("Run created: workflow='%s'",workflow->name); return run; }
workflow_engine_state_t *workflow_engine_run_state(workflow_engine_run_t *run) { return run ? &run->state : nullptr; }
const workflow_engine_state_t *workflow_engine_run_state_const(const workflow_engine_run_t *run) { return run ? &run->state : nullptr; }
workflow_engine_run_t *workflow_engine_run_next(const workflow_engine_run_t *run) { return run ? run->next : nullptr; }
workflow_engine_run_t *workflow_engine_runs_head(workflow_engine_runs_t *runs) { return runs ? runs->head : nullptr; }
workflow_engine_run_t *workflow_engine_runs_current(workflow_engine_runs_t *runs) { return runs ? runs->current : nullptr; }
void workflow_engine_runs_stop_all(workflow_engine_runs_t *runs) { if(!runs) return; for(auto *run=runs->head;run;run=run->next) workflow_engine_state_stop(&run->state); }
bool workflow_engine_runs_any_active(const workflow_engine_runs_t *runs) { if(!runs) return false; for(auto *run=runs->head;run;run=run->next) if(workflow_engine_state_is_active(&run->state)) return true; return false; }
bool workflow_engine_runs_trigger(workflow_engine_runs_t *runs, workflow_trigger_type_t type, const char *value) { if(!runs) return false; bool triggered=false; for(auto *run=runs->head;run;run=run->next) if(workflow_engine_state_is_active(&run->state) && workflow_engine_trigger_dispatch(&run->state,type,value)) triggered=true; return triggered; }
