#include <obs-frontend-api.h>

#include <QAction>
#include <QMainWindow>
#include <QTimer>

namespace {

bool is_legacy_move_workflow_action(QAction *action)
{
    if (!action)
        return false;

    const QString text = action->text();
    return text.startsWith(QStringLiteral("Move Workflow: Test "));
}

void hide_legacy_actions()
{
    auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!mainWindow)
        return;

    const auto actions = mainWindow->findChildren<QAction *>();
    for (QAction *action : actions) {
        if (is_legacy_move_workflow_action(action)) {
            action->setVisible(false);
            action->setEnabled(false);
        }
    }
}

struct MenuCleanup {
    MenuCleanup()
    {
        // The old C menu items are registered during plugin load. Keep checking
        // briefly after startup so they are hidden regardless of registration order.
        for (int delay = 0; delay <= 2000; delay += 100)
            QTimer::singleShot(delay, hide_legacy_actions);
    }
};

MenuCleanup cleanup;

} // namespace
