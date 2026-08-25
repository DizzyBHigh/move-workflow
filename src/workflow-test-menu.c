#include "workflow-test-menu.h"
#include "workflow-runtime.h"

#include <obs-frontend-api.h>

struct test_context {
    workflow_t *workflow;
    workflow_engine_t *engine;
};

static void left_cb(void *data) { workflow_runtime_execute_node_by_id(data, "move-left"); }
static void center_cb(void *data) { workflow_runtime_execute_node_by_id(data, "move-center"); }
static void right_cb(void *data) { workflow_runtime_execute_node_by_id(data, "move-right"); }

static void duration_cb(void *data)
{
    workflow_runtime_test_duration(data);
}

static void engine_cb(void *data)
{
    struct test_context *context = data;
    if (!context || !context->workflow || !context->engine)
        return;
    workflow_engine_stop(context->engine);
    if (workflow_engine_start(context->engine, context->workflow))
        workflow_engine_run_entries(context->engine);
}

void workflow_test_menu_register(workflow_t *workflow,
                                 workflow_engine_t *engine)
{
    static struct test_context context;
    context.workflow = workflow;
    context.engine = engine;
    obs_frontend_add_tools_menu_item("Move Workflow: Test Left", left_cb, workflow);
    obs_frontend_add_tools_menu_item("Move Workflow: Test Center", center_cb, workflow);
    obs_frontend_add_tools_menu_item("Move Workflow: Test Right", right_cb, workflow);
    obs_frontend_add_tools_menu_item("Move Workflow: Test Duration Override (Left)", duration_cb, workflow);
    obs_frontend_add_tools_menu_item("Move Workflow: Run Execution Engine", engine_cb, &context);
}
