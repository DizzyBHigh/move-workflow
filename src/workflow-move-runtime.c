#include "workflow-move-runtime.h"
#include "workflow-debug.h"
#include <obs.h>
#include <string.h>
#include <stdlib.h>

#define MOVE_SOURCE_FILTER_ID "move_source_filter"
#define MOVE_SOURCE_SWAP_FILTER_ID "move_source_swap_filter"
#define MOVE_VALUE_FILTER_ID "move_value_filter"
#define MOVE_ACTION_FILTER_ID "move_action_filter"

typedef struct move_hotkey_lookup {
    obs_source_t *filter;
    const char *filter_name;
    obs_hotkey_id id;
} move_hotkey_lookup_t;

typedef struct move_dispatch_task {
    obs_source_t *filter;
    obs_hotkey_id id;
} move_dispatch_task_t;

static bool supported_move_filter(const char *id)
{
    return id && (!strcmp(id, MOVE_SOURCE_FILTER_ID) ||
                  !strcmp(id, MOVE_SOURCE_SWAP_FILTER_ID) ||
                  !strcmp(id, MOVE_VALUE_FILTER_ID) ||
                  !strcmp(id, MOVE_ACTION_FILTER_ID));
}

static bool find_move_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *key)
{
    move_hotkey_lookup_t *lookup = data;
    if (obs_hotkey_get_registerer_type(key) != OBS_HOTKEY_REGISTERER_SOURCE)
        return true;
    if (strcmp(obs_hotkey_get_name(key), lookup->filter_name) != 0)
        return true;

    obs_weak_source_t *weak = obs_hotkey_get_registerer(key);
    obs_source_t *registerer = weak ? obs_weak_source_get_source(weak) : NULL;
    if (!registerer)
        return true;

    bool match = registerer == lookup->filter;
    if (match)
        lookup->id = id;
    obs_source_release(registerer);
    return !match;
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

static void dispatch_move_hotkey(void *data)
{
    move_dispatch_task_t *task = data;
    if (!task)
        return;

    obs_source_set_enabled(task->filter, true);
    workflow_debug_log("Move dispatch: triggering hotkey id=%llu enabled=%d",
                       (unsigned long long)task->id,
                       obs_source_enabled(task->filter) ? 1 : 0);
    obs_hotkey_trigger_routed_callback(task->id, true);
    obs_hotkey_trigger_routed_callback(task->id, false);
    obs_source_release(task->filter);
    free(task);
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

    move_hotkey_lookup_t lookup = {
        .filter = filter,
        .filter_name = node->action.filter_name,
        .id = OBS_INVALID_HOTKEY_ID,
    };
    obs_enum_hotkeys(find_move_hotkey, &lookup);
    if (lookup.id == OBS_INVALID_HOTKEY_ID) {
        workflow_debug_log("Move start hotkey not found: filter='%s'",
                           node->action.filter_name);
        obs_source_release(filter);
        return false;
    }

    move_dispatch_task_t *task = calloc(1, sizeof(*task));
    if (!task) {
        obs_source_release(filter);
        return false;
    }
    task->filter = obs_source_get_ref(filter);
    task->id = lookup.id;
    obs_source_release(filter);
    if (!task->filter)
        return false;

    workflow_debug_log("Move dispatch: queueing hotkey id=%llu filter='%s'",
                       (unsigned long long)task->id, node->action.filter_name);
    obs_queue_task(OBS_TASK_UI, dispatch_move_hotkey, task, false);
    return true;
}
