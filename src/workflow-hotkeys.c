#include "workflow-hotkeys.h"
#include "workflow-model.h"
#include "workflow-persistence.h"
#include "workflow-shortcuts.h"

#include <obs-module.h>
#include <stdio.h>

#define MAX_SHORTCUT_BINDINGS (WORKFLOW_MAX_NODES * WORKFLOW_MAX_LINKS)

typedef struct shortcut_binding {
    obs_hotkey_id id;
    workflow_t *workflow;
    char source_id[WORKFLOW_MAX_NAME];
    char target_id[WORKFLOW_MAX_NAME];
} shortcut_binding_t;

static shortcut_binding_t bindings[MAX_SHORTCUT_BINDINGS];
static size_t binding_count;
static obs_hotkey_id redo_hotkey_id = OBS_INVALID_HOTKEY_ID;

extern void workflow_editor_redo_from_hotkey(void);

static void shortcut_cb(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
    UNUSED_PARAMETER(id);
    UNUSED_PARAMETER(hotkey);
    shortcut_binding_t *binding = data;
    if (pressed && binding)
        workflow_shortcuts_accept(binding->workflow, binding->source_id, binding->target_id);
}

static void redo_cb(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(id);
    UNUSED_PARAMETER(hotkey);
    if (pressed)
        workflow_editor_redo_from_hotkey();
}

static void clear_shortcuts(void)
{
    for (size_t i = 0; i < binding_count; ++i) {
        if (bindings[i].id != OBS_INVALID_HOTKEY_ID)
            obs_hotkey_unregister(bindings[i].id);
    }
    binding_count = 0;
    workflow_shortcuts_cancel();
}

static void register_shortcut(workflow_t *workflow, const workflow_node_t *node, size_t index)
{
    if (!workflow || !node || index >= node->shortcut_node_count || binding_count >= MAX_SHORTCUT_BINDINGS)
        return;

    shortcut_binding_t *binding = &bindings[binding_count];
    binding->workflow = workflow;
    snprintf(binding->source_id, WORKFLOW_MAX_NAME, "%s", node->id);
    snprintf(binding->target_id, WORKFLOW_MAX_NAME, "%s", node->shortcut_node_ids[index]);

    char name[WORKFLOW_MAX_VALUE];
    char description[WORKFLOW_MAX_VALUE];
    snprintf(name, sizeof(name), "move_workflow.shortcut.%s.%s", node->id, binding->target_id);
    snprintf(description, sizeof(description), "Move Workflow: %s -> %s", node->id, binding->target_id);
    binding->id = obs_hotkey_register_frontend(name, description, shortcut_cb, binding);
    if (binding->id == OBS_INVALID_HOTKEY_ID)
        return;

    obs_key_combination_t combo = {0};
    combo.modifiers = node->shortcut_modifiers[index];
    combo.key = (obs_key_t)node->shortcut_key[index];
    if (combo.key != OBS_KEY_NONE)
        obs_hotkey_load_bindings(binding->id, &combo, 1);
    ++binding_count;
}

void workflow_hotkeys_set_redo_callback(workflow_redo_callback_t callback)
{
    UNUSED_PARAMETER(callback);
}

void workflow_hotkeys_register(void)
{
    if (redo_hotkey_id == OBS_INVALID_HOTKEY_ID)
        redo_hotkey_id = obs_hotkey_register_frontend("move_workflow.redo", "Move Workflow: Redo", redo_cb, NULL);
    obs_key_combination_t combo = {0};
    combo.modifiers = INTERACT_CONTROL_KEY;
    combo.key = OBS_KEY_Y;
    obs_hotkey_load_bindings(redo_hotkey_id, &combo, 1);
    workflow_hotkeys_refresh();
}

void workflow_hotkeys_refresh(void)
{
    clear_shortcuts();
    workflow_manager_t *manager = workflow_persistence_manager();
    if (!manager)
        return;
    for (size_t w = 0; w < manager->workflow_count; ++w) {
        workflow_t *workflow = &manager->workflows[w];
        if (!workflow->enabled)
            continue;
        for (size_t i = 0; i < workflow->node_count; ++i) {
            const workflow_node_t *node = &workflow->nodes[i];
            for (size_t j = 0; j < node->shortcut_node_count; ++j)
                register_shortcut(workflow, node, j);
        }
    }
}

void workflow_hotkeys_unregister(void)
{
    clear_shortcuts();
    if (redo_hotkey_id != OBS_INVALID_HOTKEY_ID) {
        obs_hotkey_unregister(redo_hotkey_id);
        redo_hotkey_id = OBS_INVALID_HOTKEY_ID;
    }
}
