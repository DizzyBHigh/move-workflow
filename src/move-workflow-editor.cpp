#include "move-workflow-editor.h"
#include "workflow-editor-window.h"

#include <QTimer>

namespace {

void register_menu()
{
    QTimer::singleShot(0, [] { show_move_workflow_editor(); });
}

struct AutoRegister {
    AutoRegister() { QTimer::singleShot(0, register_menu); }
};

AutoRegister auto_register;

} // namespace

void move_workflow_register_editor(void)
{
    QTimer::singleShot(0, register_menu);
}
