#include "workflow-action-executor.hpp"
#include "workflow-change-scene.h"
#include "workflow-filter-instance.h"
#include "workflow-runtime.h"
namespace { workflow_execution_mode execution_mode=workflow_execution_mode::temporary_instance; }
void workflow_action_executor_set_mode(workflow_execution_mode mode){execution_mode=mode;}
workflow_execution_mode workflow_action_executor_get_mode(){return execution_mode;}
bool workflow_action_executor_execute(workflow_t*w,workflow_node_t*n){if(!w||!n)return false;if(n->action.kind==WORKFLOW_CHANGE_SCENE)return workflow_change_scene_execute(n);switch(execution_mode){case workflow_execution_mode::temporary_instance:return workflow_filter_instance_execute_node(w,n);case workflow_execution_mode::legacy_runtime:return workflow_runtime_execute_node_by_id(w,n->id);}return false;}
