#include "workflow-engine.h"
#include "workflow-debug.h"
#include "workflow-engine-node.h"
#include "workflow-engine-runner.h"
#include "workflow-engine-runs.h"
#include "workflow-engine-trigger.h"
#include <cstdlib>
#include <cstring>

struct workflow_engine { workflow_engine_runs_t *runs; };
workflow_engine_t *workflow_engine_create(void) { auto *e=(workflow_engine_t *)calloc(1,sizeof(*e)); if(!e) return nullptr; e->runs=workflow_engine_runs_create(); if(!e->runs){free(e);return nullptr;} return e; }
void workflow_engine_destroy(workflow_engine_t *e) { if(!e)return; workflow_engine_stop(e); workflow_engine_runs_destroy(e->runs); free(e); }
bool workflow_engine_start(workflow_engine_t *e, workflow_t *w) { if(!e||!w)return false; auto *r=workflow_engine_runs_start(e->runs,w); return r!=nullptr; }
bool workflow_engine_start_trigger(workflow_engine_t *e, workflow_t *w, const char *id) { if(!e||!w||!id||!*id||!w->enabled)return false; auto *n=workflow_engine_find_node(w,id); if(!n||n->type!=WORKFLOW_NODE_TRIGGER)return false; auto *r=workflow_engine_runs_start(e->runs,w); if(!r)return false; workflow_debug_log("External trigger started workflow='%s' node='%s'",w->name,id); return workflow_engine_runner_run_node(workflow_engine_run_state(r),id); }
void workflow_engine_stop(workflow_engine_t *e) { if(e)workflow_engine_runs_stop_all(e->runs); }
bool workflow_engine_is_running(const workflow_engine_t *e) { return e&&workflow_engine_runs_any_active(e->runs); }
bool workflow_engine_is_workflow_running(const workflow_engine_t *e,const char *id) { if(!e||!id)return false; for(auto *r=workflow_engine_runs_head(e->runs);r;r=workflow_engine_run_next(r)){const auto *s=workflow_engine_run_state_const(r); if(workflow_engine_state_is_active(s)&&s->workflow&&!strcmp(s->workflow->id,id))return true;} return false; }
static workflow_engine_state_t *current_state(workflow_engine_t *e) { return e?workflow_engine_run_state(workflow_engine_runs_current(e->runs)):nullptr; }
bool workflow_engine_run_entries(workflow_engine_t *e) { return workflow_engine_runner_run_entries(current_state(e)); }
bool workflow_engine_run_node(workflow_engine_t *e,const char *id) { return workflow_engine_runner_run_node(current_state(e),id); }
bool workflow_engine_test_node(workflow_engine_t *e,workflow_t *w,const char *id) { if(!e||!w||!id||!*id)return false; auto *r=workflow_engine_runs_start(e->runs,w); if(!r)return false; return workflow_engine_runner_run_node(workflow_engine_run_state(r),id); }
bool workflow_engine_trigger(workflow_engine_t *e,workflow_trigger_type_t type,const char *value) { return e&&workflow_engine_runs_trigger(e->runs,type,value); }
