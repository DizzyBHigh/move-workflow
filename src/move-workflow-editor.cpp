#include "move-workflow-editor.h"
#include "workflow-editor-window.h"
#include "workflow-monitor-window.hpp"

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

    QAction *monitor = static_cast<QAction *>(
        obs_frontend_add_tools_menu_qaction("Move Workflow Monitor"));
    if (!monitor)
        return;
    QObject::connect(monitor, &QAction::triggered, [] {
        workflow_monitor_window::show(obs_frontend_get_main_window());
    });
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
