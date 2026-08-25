#include "workflow-move-runtime.h"

#include <obs.h>
#include <obs-module.h>
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
    char *scene_name;
    char *filter_name;
} move_dispatch_task_t;

static bool is_supported_move_filter(const char *id)
{
    return id && (!strcmp(id, MOVE_SOURCE_FILTER_ID) || !strcmp(id, MOVE_SOURCE_SWAP_FILTER_ID) ||
                  !strcmp(id, MOVE_VALUE_FILTER_ID) || !strcmp(id, MOVE_ACTION_FILTER_ID));
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
    if (match) {
        lookup->id = id;
        blog(LOG_INFO,
             "[Move Workflow][Debug] Move hotkey match: id=%llu filter='%s' registerer='%p'",
             (unsigned long long)id, lookup->filter_name, (void *)registerer);
    }
    obs_source_release(registerer);
    return !match;
}

static obs_source_t *find_target_filter(const workflow_action_ref_t *action)
{
    if (!action || !action->scene_name[0] || !action->filter_name[0])
        return NULL;
    if (!is_supported_move_filter(action->filter_id))
        return NULL;
    obs_source_t *scene = obs_get_source_by_name(action->scene_name);
    if (!scene)
        return NULL;
    obs_source_t *filter = obs_source_get_filter_by_name(scene, action->filter_name);
    obs_source_release(scene);
    if (!filter)
        return NULL;
    if (strcmp(obs_source_get_unversioned_id(filter), action->filter_id) != 0) {
        obs_source_release(filter);
        return NULL;
    }
    return filter;
}

static void run_move_dispatch(void *data)
{
    move_dispatch_task_t *task = data;
    if (!task)
        return;

    if (!obs_source_enabled(task->filter))
        obs_source_set_enabled(task->filter, true);

    blog(LOG_INFO,
         "[Move Workflow][Debug] Move dispatch: UI task triggering hotkey id=%llu scene='%s' filter='%s' enabled=%d",
         (unsigned long long)task->id, task->scene_name, task->filter_name,
         obs_source_enabled(task->filter) ? 1 : 0);

    obs_hotkey_trigger_routed_callback(task->id, true);
    obs_hotkey_trigger_routed_callback(task->id, false);

    blog(LOG_INFO,
         "[Move Workflow][Debug] Move dispatch: UI task completed scene='%s' filter='%s'",
         task->scene_name, task->filter_name);

    obs_source_release(task->filter);
    bfree(task->scene_name);
    bfree(task->filter_name);
    free(task);
}

bool workflow_move_runtime_trigger(workflow_t *workflow, workflow_node_t *node)
{
    if (!workflow || !workflow->enabled || !node || node->type != WORKFLOW_NODE_ACTION)
        return false;

    obs_source_t *filter = find_target_filter(&node->action);
    if (!filter) {
        blog(LOG_WARNING, "[Move Workflow] Move target not found: scene='%s' filter='%s' id='%s'",
             node->action.scene_name, node->action.filter_name, node->action.filter_id);
        return false;
    }

    move_hotkey_lookup_t lookup = {
        .filter = filter,
        .filter_name = node->action.filter_name,
        .id = OBS_INVALID_HOTKEY_ID,
    };
    obs_enum_hotkeys(find_move_hotkey, &lookup);
    if (lookup.id == OBS_INVALID_HOTKEY_ID) {
        blog(LOG_WARNING,
             "[Move Workflow] Move start hotkey not found on target filter: scene='%s' filter='%s'.",
             node->action.scene_name, node->action.filter_name);
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
    task->scene_name = bstrdup(node->action.scene_name);
    task->filter_name = bstrdup(node->action.filter_name);
    obs_source_release(filter);

    if (!task->filter || !task->scene_name || !task->filter_name) {
        if (task->filter)
            obs_source_release(task->filter);
        bfree(task->scene_name);
        bfree(task->filter_name);
        free(task);
        return false;
    }

    blog(LOG_INFO,
         "[Move Workflow][Debug] Move dispatch: queueing UI task id=%llu scene='%s' filter='%s'",
         (unsigned long long)lookup.id, node->action.scene_name, node->action.filter_name);
    obs_queue_task(OBS_TASK_UI, run_move_dispatch, task, false);
    return true;
}
