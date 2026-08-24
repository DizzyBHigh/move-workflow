#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include "workflow-hotkeys.h"
#include "workflow-runtime.h"
#include "workflow-test-menu.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
    workflow_t *workflow = workflow_runtime_test_workflow();
    blog(LOG_INFO, "[Move Workflow] Loaded");
    workflow_hotkeys_register();
    workflow_test_menu_register(workflow);
    return true;
}

void obs_module_unload(void)
{
    workflow_hotkeys_unregister();
    blog(LOG_INFO, "[Move Workflow] Unloaded");
}
