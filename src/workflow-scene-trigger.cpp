#include "workflow-scene-trigger.h"

#include "workflow-engine-service.h"

#include <obs-frontend-api.h>
#include <obs.h>

namespace {
static void scene_changed(enum obs_frontend_event event, void *)
{
	if (event != OBS_FRONTEND_EVENT_SCENE_CHANGED)
		return;
	obs_source_t *scene = obs_frontend_get_current_scene();
	if (!scene)
		return;
	workflow_engine_service_trigger_scene(obs_source_get_name(scene));
	obs_source_release(scene);
}

static bool registered;
}

void workflow_scene_trigger_register(void)
{
	if (registered)
		return;
	obs_frontend_add_event_callback(scene_changed, nullptr);
	registered = true;
}

void workflow_scene_trigger_unregister(void)
{
	if (!registered)
		return;
	obs_frontend_remove_event_callback(scene_changed, nullptr);
	registered = false;
}
