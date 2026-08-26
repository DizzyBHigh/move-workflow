#include "workflow-change-scene.h"
#include <obs-frontend-api.h>

bool workflow_change_scene_execute(const workflow_node_t *node)
{
	if (!node || node->action.kind != WORKFLOW_CHANGE_SCENE || !node->action.scene_name[0])
		return false;
	obs_source_t *scene = obs_get_source_by_name(node->action.scene_name);
	if (!scene)
		return false;
	obs_frontend_set_current_scene(scene);
	obs_source_release(scene);
	return true;
}

uint64_t workflow_change_scene_transition_duration(void)
{
	return (uint64_t)obs_frontend_get_transition_duration();
}
