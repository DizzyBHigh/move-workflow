#include "workflow-engine-service.h"
#include "workflow-engine.h"
#include "workflow-engine-node.h"
#include "workflow-persistence.h"
#include <obs-module.h>
static workflow_engine_t *service_engine;
void workflow_engine_service_set(workflow_engine_t *engine){service_engine=engine;blog(LOG_INFO,"[Move Workflow] Engine service %s.",engine?"connected":"disconnected");}
static workflow_t *find_workflow(const char *id){auto*m=workflow_persistence_manager();return m&&id?workflow_manager_find(m,id):nullptr;}
bool workflow_engine_service_trigger(const char *workflow_id,const char *trigger_id){if(!service_engine||!workflow_id||!trigger_id)return false;auto*w=find_workflow(workflow_id);if(!w)return false;auto*n=workflow_engine_find_node(w,trigger_id);if(!n||n->type!=WORKFLOW_NODE_TRIGGER)return false;return workflow_engine_start_trigger(service_engine,w,n->id);}
bool workflow_engine_service_trigger_scene(const char *){return false;}
bool workflow_engine_service_workflow_running(const char *workflow_id){return service_engine&&workflow_id&&workflow_engine_is_workflow_running(service_engine,workflow_id);}
bool workflow_engine_service_test_node(const char *workflow_id,const char *node_id){if(!service_engine||!workflow_id||!node_id)return false;auto*w=find_workflow(workflow_id);if(!w||!w->enabled)return false;return workflow_engine_test_node(service_engine,w,node_id);}
