#include "workflow-hotkeys.hpp"
#include <obs-frontend-api.h>

static obs_hotkey_id redo_hotkey_id = OBS_INVALID_HOTKEY_ID;

static void handle_redo_action(void *data, obs_hotkey_id id,
                               obs_hotkey_t *hotkey, bool pressed)
{
    (void)data;
    (void)id;
    (void)hotkey;
    if (!pressed)
        return;

    // Debugging recipe retained for future OBS hotkey diagnostics:
    // blog(LOG_INFO,
    //      "[obs-move-workflow] Ctrl+Y intercepted via OBS Hotkey API.");

    extern void workflow_editor_redo_from_hotkey(void);
    workflow_editor_redo_from_hotkey();
}

void RegisterWorkflowHotkeys(void *data)
{
    redo_hotkey_id = obs_hotkey_register_frontend(
        "move_workflow_redo",
        "Move Workflow: Redo",
        handle_redo_action,
        data);

    struct obs_key_combination combo = {};
    combo.modifiers = INTERACTION_CONTROL;
    combo.key = OBS_KEY_Y;

    obs_hotkey_register_binding(redo_hotkey_id, combo);
}
