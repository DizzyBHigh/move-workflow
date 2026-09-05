#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*workflow_redo_callback_t)(void);

void workflow_hotkeys_register(void);
void workflow_hotkeys_refresh(void);
void workflow_hotkeys_unregister(void);
void workflow_hotkeys_set_redo_callback(workflow_redo_callback_t callback);

#ifdef __cplusplus
}
#endif
