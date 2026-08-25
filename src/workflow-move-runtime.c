#include "workflow-move-runtime.h"
#include "workflow-debug.h"
#include <obs.h>
#include <string.h>

#define MOVE_SOURCE_FILTER_ID "move_source_filter"
#define MOVE_SOURCE_SWAP_FILTER_ID "move_source_swap_filter"
#define MOVE_VALUE_FILTER_ID "move_value_filter"
#define MOVE_ACTION_FILTER_ID "move_action_filter"
#define MOVE_START_TRIGGER "start_trigger"
#define MOVE_START_TRIGGER_LOAD 13
#define MOVE_SIMULTANEOUS_MOVE "simultaneous_move"
#define MOVE_NEXT_MOVE "next_move"
#define MOVE_NEXT_MOVE_ON "next_move_on"
#define MOVE_NEXT_MOVE_ON_END 0

static bool supported_move_filter(const char *id)
{
    return id && (!strcmp(id, MOVE_SOURCE_FILTER_ID) ||
                  !strcmp(id, MOVE_SOURCE_SWAP_FILTER_ID) ||
                  !strcmp(id, MOVE_VALUE_FILTER_ID) ||
                  !strcmp(id, MOVE_ACTION_FILTER_ID));
}

static obs_source_t *find_target_filter(const workflow_action_ref_t *action)
{
    if (!action || !action->scene_name[0] || !action->filter_name[0] ||
        !supported_move_filter(action->filter_id))
        return NULL;

    obs_source_t *scene = obs_get_source_by_name(action->scene_name);
    if (!scene)
        return NULL;
    obs_source_t *filter = obs_source_get_filter_by_name(scene, action->filter_name);
    obs_source_release(scene);
    if (!filter)
        return NULL;

    const char *actual_id = obs_source_get_unversioned_id(filter);
    if (!actual_id || strcmp(actual_id, action->filter_id) != 0) {
        obs_source_release(filter);
        return NULL;
    }
    return filter;
}

static bool start_move_filter(obs_source_t *filter, const char *filter_name)
{
    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings)
        return false;

    const int original_trigger = (int)obs_data_get_int(settings, MOVE_START_TRIGGER);
    const char *original_simultaneous = obs_data_get_string(settings, MOVE_SIMULTANEOUS_MOVE);
    const char *original_next = obs_data_get_string(settings, MOVE_NEXT_MOVE);
    const int original_next_on = (int)obs_data_get_int(settings, MOVE_NEXT_MOVE_ON);
    char simultaneous_copy[256];
    char next_copy[256];
    strncpy(simultaneous_copy, original_simultaneous ? original_simultaneous : "", sizeof(simultaneous_copy) - 1);
    simultaneous_copy[sizeof(simultaneous_copy) - 1] = '\0';
    strncpy(next_copy, original_next ? original_next : "", sizeof(next_copy) - 1);
    next_copy[sizeof(next_copy) - 1] = '\0';

    obs_data_set_string(settings, MOVE_SIMULTANEOUS_MOVE, "");
    obs_data_set_string(settings, MOVE_NEXT_MOVE, "");
    obs_data_set_int(settings, MOVE_NEXT_MOVE_ON, MOVE_NEXT_MOVE_ON_END);
    obs_data_set_int(settings, MOVE_START_TRIGGER, MOVE_START_TRIGGER_LOAD);

    workflow_debug_log("Move dispatch: suppressing native chaining for filter='%s'", filter_name);
    workflow_debug_log("Move dispatch: forcing LOAD trigger for filter='%s' original=%d",
                       filter_name, original_trigger);
    obs_source_update(filter, settings);

    obs_data_set_string(settings, MOVE_SIMULTANEOUS_MOVE, simultaneous_copy);
    obs_data_set_string(settings, MOVE_NEXT_MOVE, next_copy);
    obs_data_set_int(settings, MOVE_NEXT_MOVE_ON, original_next_on);
    obs_data_set_int(settings, MOVE_START_TRIGGER, original_trigger);
    obs_source_update(filter, settings);
    obs_data_release(settings);

    workflow_debug_log("Move dispatch: native Move update completed for filter='%s'",
                       filter_name);
    return true;
}

bool workflow_move_runtime_trigger(workflow_t *workflow, workflow_node_t *node)
{
    if (!workflow || !workflow->enabled || !node || node->type != WORKFLOW_NODE_ACTION)
        return false;

    obs_source_t *filter = find_target_filter(&node->action);
    if (!filter) {
        workflow_debug_log("Move target lookup failed: scene='%s' filter='%s'",
                           node->action.scene_name, node->action.filter_name);
        return false;
    }

    const bool started = start_move_filter(filter, node->action.filter_name);
    obs_source_release(filter);
    return started;
}
