#include "workflow-engine-runner-shortcuts.h"

#include "workflow-debug.h"
#include "workflow-engine-node.h"
#include "workflow-engine-runner-internal.h"

#include <cstring>

bool workflow_engine_runner_wait_shortcut(workflow_engine_state_t *state,
                                          workflow_node_t *node)
{
    if (!state || !node || !node->shortcut_node_count)
        return false;
    state->waiting_for_shortcut = true;
    std::strncpy(state->shortcut_source_id, node->id, WORKFLOW_MAX_NAME - 1);
    state->shortcut_source_id[WORKFLOW_MAX_NAME - 1] = '\0';
    workflow_engine_state_delay_begin(state);
    workflow_debug_log("Shortcut: workflow='%s' waiting at node='%s' for a shortcut",
                       state->workflow ? state->workflow->name : "", node->id);
    return true;
}

static bool target_is_shortcut(const workflow_node_t *node, const char *target_id)
{
    if (!node || !target_id)
        return false;
    for (size_t i = 0; i < node->shortcut_node_count; ++i)
        if (!std::strcmp(node->shortcut_node_ids[i], target_id))
            return true;
    return false;
}

static const char *shortcut_key_for_target(const workflow_node_t *node,
                                           const char *target_id)
{
    if (!node || !target_id)
        return nullptr;
    for (size_t i = 0; i < node->shortcut_binding_count; ++i) {
        const workflow_shortcut_binding_t *binding = &node->shortcut_bindings[i];
        if (!std::strcmp(binding->target_id, target_id))
            return binding->key;
    }
    return nullptr;
}

bool workflow_engine_runner_activate_shortcut(workflow_engine_state_t *state,
                                              const char *source_id,
                                              const char *target_id)
{
    if (!state || !workflow_engine_state_is_active(state) || !source_id || !target_id)
        return false;
    if (!state->waiting_for_shortcut || std::strcmp(state->shortcut_source_id, source_id))
        return false;

    workflow_node_t *source = workflow_engine_find_node(state->workflow, source_id);
    if (!source || !target_is_shortcut(source, target_id))
        return false;

    const char *pressed_key = shortcut_key_for_target(source, target_id);
    size_t activated = 0;
    bool result = false;

    state->waiting_for_shortcut = false;
    state->shortcut_source_id[0] = '\0';
    workflow_debug_log("Shortcut: workflow='%s' selected '%s' -> '%s'%s%s",
                       state->workflow ? state->workflow->name : "", source_id, target_id,
                       pressed_key ? " key='" : "", pressed_key ? pressed_key : "");

    for (size_t i = 0; i < source->shortcut_node_count; ++i) {
        const char *candidate_id = source->shortcut_node_ids[i];
        if (pressed_key) {
            const char *candidate_key = shortcut_key_for_target(source, candidate_id);
            if (!candidate_key || std::strcmp(candidate_key, pressed_key) != 0)
                continue;
        } else if (std::strcmp(candidate_id, target_id) != 0) {
            continue;
        }

        const bool candidate_result =
            workflow_engine_runner_run_internal(state, candidate_id, 0);
        result = candidate_result || result;
        ++activated;
        workflow_debug_log("Shortcut: activated target '%s' result=%d",
                           candidate_id, candidate_result);
    }

    workflow_engine_state_delay_end(state);
    workflow_debug_log("Shortcut: workflow='%s' source='%s' activated=%zu result=%d",
                       state->workflow ? state->workflow->name : "", source_id,
                       activated, result);
    return activated != 0 && result;
}
