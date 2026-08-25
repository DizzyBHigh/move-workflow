#include "workflow-move-runtime.h"

#include <obs.h>
#include <obs-module.h>
#include <string.h>

#define MOVE_SOURCE_FILTER_ID "move_source_filter"
#define MOVE_SOURCE_SWAP_FILTER_ID "move_source_swap_filter"
#define MOVE_VALUE_FILTER_ID "move_value_filter"
#define MOVE_ACTION_FILTER_ID "move_action_filter"

static bool is_supported_move_filter(const char *id)
{
    return id && (!strcmp(id, MOVE_SOURCE_FILTER_ID) || !strcmp(id, MOVE_SOURCE_SWAP_FILTER_ID) ||
                  !strcmp(id, MOVE_VALUE_FILTER_ID) || !strcmp(id, MOVE_ACTION_FILTER_ID));
}

typedef struct move_hotkey_lookup {
    const char *scene_name;
    const char *filter_name;
    obs_hotkey_id id;
} move_hotkey_lookup_t;

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

    const char *name = obs_source_get_name(registerer);
    if (name && strcmp(name, lookup->scene_name) == 0) {
        lookup->id = id;
        obs_source_release(registerer);
        return false;
    }

    obs_source_release(registerer);
    return true;
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

    const bool was_enabled = obs_source_enabled(filter);
    obs_source_set_enabled(filter, true);
    blog(LOG_INFO, "[Move Workflow][Debug] Move dispatch: scene='%s' filter='%s' enabled=%d->1",
         node->action.scene_name, node->action.filter_name, was_enabled ? 1 : 0);
    obs_source_release(filter);

    move_hotkey_lookup_t lookup = {
        .scene_name = node->action.scene_name,
        .filter_name = node->action.filter_name,
        .id = OBS_INVALID_HOTKEY_ID,
    };
    obs_enum_hotkeys(find_move_hotkey, &lookup);
    if (lookup.id == OBS_INVALID_HOTKEY_ID) {
        blog(LOG_WARNING, "[Move Workflow] Move hotkey not found: scene='%s' filter='%s'",
             node->action.scene_name, node->action.filter_name);
        return false;
    }

    blog(LOG_INFO, "[Move Workflow][Debug] Move dispatch: triggering registered Move hotkey id=%llu",
         (unsigned long long)lookup.id);
    obs_hotkey_trigger_routed_callback(lookup.id, true);
    return true;
}
