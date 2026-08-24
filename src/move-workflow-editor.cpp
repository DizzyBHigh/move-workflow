#include "move-workflow-editor.h"
#include "workflow-editor-window.h"

#include <obs-frontend-api.h>

#include <QAction>
#include <QTimer>

namespace {

void register_menu()
{
    QAction *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction("Move Workflow Editor"));
    if (!action)
        return;
    QObject::connect(action, &QAction::triggered, [] { show_move_workflow_editor(); });
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
