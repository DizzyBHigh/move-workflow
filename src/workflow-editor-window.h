#pragma once

class QWidget;

void show_move_workflow_editor(QWidget *parent = nullptr);
void destroy_move_workflow_editor(void);

#ifdef __cplusplus
extern "C" {
#endif
void workflow_editor_redo_from_hotkey(void);
#ifdef __cplusplus
}
#endif
