#include "workflow-hotkeys.h"
#include "workflow-model.h"
#include "workflow-persistence.h"
#include "workflow-shortcuts.h"

#include <obs-module.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

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
    UNUSED_PARAMETER(id); UNUSED_PARAMETER(hotkey);
    shortcut_binding_t *binding = data;
    if (pressed && binding)
        workflow_shortcuts_accept(binding->workflow, binding->source_id, binding->target_id);
}

static void redo_cb(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
    UNUSED_PARAMETER(data); UNUSED_PARAMETER(id); UNUSED_PARAMETER(hotkey);
    if (pressed) workflow_editor_redo_from_hotkey();
}

static obs_key_t shortcut_key_from_token(const char *token)
{
    static const struct { const char *text; const char *name; } special[] = {
        {"Return", "OBS_KEY_RETURN"}, {"Enter", "OBS_KEY_ENTER"},
        {"Escape", "OBS_KEY_ESCAPE"}, {"Tab", "OBS_KEY_TAB"},
        {"Backspace", "OBS_KEY_BACKSPACE"}, {"Delete", "OBS_KEY_DELETE"},
        {"Insert", "OBS_KEY_INSERT"}, {"Home", "OBS_KEY_HOME"},
        {"End", "OBS_KEY_END"}, {"Left", "OBS_KEY_LEFT"},
        {"Right", "OBS_KEY_RIGHT"}, {"Up", "OBS_KEY_UP"},
        {"Down", "OBS_KEY_DOWN"}, {"PageUp", "OBS_KEY_PAGEUP"},
        {"PageDown", "OBS_KEY_PAGEDOWN"}, {"Space", "OBS_KEY_SPACE"}
    };
    char name[32];
    size_t len;
    int function;
    if (!token || !*token) return OBS_KEY_NONE;
    for (size_t i = 0; i < sizeof(special) / sizeof(special[0]); ++i)
        if (strcmp(token, special[i].text) == 0)
            return obs_key_from_name(special[i].name);
    len = strlen(token);
    if (len >= 2 && (token[0] == 'F' || token[0] == 'f')) {
        function = atoi(token + 1);
        if (function >= 1 && function <= 35) {
            snprintf(name, sizeof(name), "OBS_KEY_F%d", function);
            return obs_key_from_name(name);
        }
    }
    if (len == 1 && ((token[0] >= 'A' && token[0] <= 'Z') ||
                     (token[0] >= 'a' && token[0] <= 'z') ||
                     (token[0] >= '0' && token[0] <= '9'))) {
        snprintf(name, sizeof(name), "OBS_KEY_%c", (char)toupper((unsigned char)token[0]));
        return obs_key_from_name(name);
    }
    return OBS_KEY_NONE;
}

static bool parse_shortcut(const char *text, obs_key_combination_t *combo)
{
    char buffer[WORKFLOW_MAX_NAME];
    char *token;
    if (!text || !combo || !*text) return false;
    memset(combo, 0, sizeof(*combo));
    snprintf(buffer, sizeof(buffer), "%s", text);
    token = strtok(buffer, "+");
    while (token) {
        while (*token == ' ') ++token;
        if (strcmp(token, "Ctrl") == 0 || strcmp(token, "Control") == 0)
            combo->modifiers |= INTERACT_CONTROL_KEY;
        else if (strcmp(token, "Alt") == 0)
            combo->modifiers |= INTERACT_ALT_KEY;
        else if (strcmp(token, "Shift") == 0)
            combo->modifiers |= INTERACT_SHIFT_KEY;
        else if (strcmp(token, "Meta") == 0 || strcmp(token, "Command") == 0)
            combo->modifiers |= INTERACT_COMMAND_KEY;
        else
            combo->key = shortcut_key_from_token(token);
        token = strtok(NULL, "+");
    }
    return combo->key != OBS_KEY_NONE;
}

static void register_shortcut(workflow_t *workflow, const char *source_id,
                              const char *target_id, const char *key)
{
    if (!workflow || !source_id || !target_id || !key || !*key ||
        binding_count >= MAX_SHORTCUT_BINDINGS)
        return;
    obs_key_combination_t combo;
    if (!parse_shortcut(key, &combo)) return;
    shortcut_binding_t *binding = &bindings[binding_count++];
    binding->workflow = workflow;
    snprintf(binding->source_id, WORKFLOW_MAX_NAME, "%s", source_id);
    snprintf(binding->target_id, WORKFLOW_MAX_NAME, "%s", target_id);
    char name[WORKFLOW_MAX_VALUE], description[WORKFLOW_MAX_VALUE];
    snprintf(name, sizeof(name), "move_workflow.shortcut.%s.%s", source_id, target_id);
    snprintf(description, sizeof(description), "Move Workflow: %s -> %s (%s)",
             source_id, target_id, key);
    binding->id = obs_hotkey_register_frontend(name, description, shortcut_cb, binding);
    if (binding->id != OBS_INVALID_HOTKEY_ID)
        obs_hotkey_load_bindings(binding->id, &combo, 1);
}

void workflow_hotkeys_set_redo_callback(workflow_redo_callback_t callback)
{
    UNUSED_PARAMETER(callback);
}

void workflow_hotkeys_register(void)
{
    redo_hotkey_id = obs_hotkey_register_frontend("move_workflow.redo", "Move Workflow: Redo", redo_cb, NULL);
    obs_key_combination_t combo = {0};
    combo.modifiers = INTERACT_CONTROL_KEY; combo.key = OBS_KEY_Y;
    obs_hotkey_load_bindings(redo_hotkey_id, &combo, 1);
    binding_count = 0;
    workflow_manager_t *manager = workflow_persistence_manager();
    workflow_t *workflow = workflow_manager_selected(manager);
    if (!workflow) return;
    for (size_t i = 0; i < workflow->node_count; ++i) {
        workflow_node_t *node = &workflow->nodes[i];
        for (size_t j = 0; j < node->shortcut_binding_count; ++j) {
            const workflow_shortcut_binding_t *b = &node->shortcut_bindings[j];
            register_shortcut(workflow, node->id, b->target_id, b->key);
        }
    }
}

void workflow_hotkeys_unregister(void)
{
    for (size_t i = 0; i < binding_count; ++i)
        if (bindings[i].id != OBS_INVALID_HOTKEY_ID)
            obs_hotkey_unregister(bindings[i].id);
    if (redo_hotkey_id != OBS_INVALID_HOTKEY_ID) {
        obs_hotkey_unregister(redo_hotkey_id);
        redo_hotkey_id = OBS_INVALID_HOTKEY_ID;
    }
    binding_count = 0;
    workflow_shortcuts_cancel();
}