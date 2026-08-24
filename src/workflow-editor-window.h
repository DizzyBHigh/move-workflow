#pragma once

class QWidget;

void show_move_workflow_editor(QWidget *parent = nullptr);

#ifdef __cplusplus
extern "C" {
#endif
void workflow_editor_redo_from_hotkey(void);
#ifdef __cplusplus
}
#endif
