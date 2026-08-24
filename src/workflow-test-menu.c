#include "workflow-test-menu.h"
#include "workflow-runtime.h"

#include <obs-frontend-api.h>

static void left_cb(void *data) { workflow_runtime_execute_node_by_id(data, "move-left"); }
static void center_cb(void *data) { workflow_runtime_execute_node_by_id(data, "move-center"); }
static void right_cb(void *data) { workflow_runtime_execute_node_by_id(data, "move-right"); }

static void duration_cb(void *data)
{
    workflow_runtime_test_duration(data);
}

void workflow_test_menu_register(workflow_t *workflow)
{
    obs_frontend_add_tools_menu_item("Move Workflow: Test Left", left_cb, workflow);
    obs_frontend_add_tools_menu_item("Move Workflow: Test Center", center_cb, workflow);
    obs_frontend_add_tools_menu_item("Move Workflow: Test Right", right_cb, workflow);
    obs_frontend_add_tools_menu_item("Move Workflow: Test Duration Override (Left)", duration_cb, workflow);
}
