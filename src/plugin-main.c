#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include "workflow-engine.h"
#include "workflow-engine-service.h"
#include "workflow-hotkeys.h"
#include "workflow-persistence.h"
#include "workflow-runtime.h"
#include "workflow-test-menu.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static workflow_engine_t *engine;

bool obs_module_load(void)
{
    workflow_persistence_init();
    workflow_t *workflow = workflow_runtime_test_workflow();
    engine = workflow_engine_create();
    if (!engine)
        return false;
    workflow_engine_service_set(engine);
    blog(LOG_INFO, "[Move Workflow] Loaded");
    workflow_hotkeys_register();
    workflow_test_menu_register(workflow, engine);
    return true;
}

void obs_module_unload(void)
{
    workflow_persistence_save();
    workflow_hotkeys_unregister();
    workflow_engine_service_set(NULL);
    workflow_engine_destroy(engine);
    engine = NULL;
    blog(LOG_INFO, "[Move Workflow] Unloaded");
}
