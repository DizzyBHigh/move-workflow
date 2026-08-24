#include "workflow-hotkeys.h"
#include "workflow-runtime.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

static void left_cb(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
    UNUSED_PARAMETER(id); UNUSED_PARAMETER(hotkey);
    if (pressed) workflow_runtime_execute_node_by_id(data, "move-left");
}

static void center_cb(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
    UNUSED_PARAMETER(id); UNUSED_PARAMETER(hotkey);
    if (pressed) workflow_runtime_execute_node_by_id(data, "move-center");
}

static void right_cb(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
    UNUSED_PARAMETER(id); UNUSED_PARAMETER(hotkey);
    if (pressed) workflow_runtime_execute_node_by_id(data, "move-right");
}

void workflow_hotkeys_register(void)
{
    workflow_t *workflow = workflow_runtime_test_workflow();
    obs_hotkey_register_frontend("obs_move_workflow.test_left", "Move Workflow: Test Left", left_cb, workflow);
    obs_hotkey_register_frontend("obs_move_workflow.test_center", "Move Workflow: Test Center", center_cb, workflow);
    obs_hotkey_register_frontend("obs_move_workflow.test_right", "Move Workflow: Test Right", right_cb, workflow);
}

void workflow_hotkeys_unregister(void)
{
}
