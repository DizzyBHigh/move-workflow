#pragma once

#ifdef __cplusplus
class QWidget;
void show_move_workflow_editor(QWidget *parent = nullptr);
extern "C" {
#else
void show_move_workflow_editor(void *parent);
#endif
void destroy_move_workflow_editor(void);
void workflow_editor_redo_from_hotkey(void);
#ifdef __cplusplus
}
#endif
