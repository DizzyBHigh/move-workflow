#include "move-workflow-tests.h"

#include <obs.h>
#include <obs-frontend-api.h>

#include <QAction>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPointer>
#include <QMainWindow>
#include <QStringList>

#include <cstdint>

namespace {

constexpr const char *TEST_SCENE = "obs-move-workflow test scene";

obs_source_t *find_filter(const char *name)
{
    obs_source_t *scene = obs_get_source_by_name(TEST_SCENE);
    if (!scene)
        return nullptr;

    obs_source_t *filter = obs_source_get_filter_by_name(scene, name);
    obs_source_release(scene);
    return filter;
}

void trigger_filter(const char *name, const char *label)
{
    obs_source_t *filter = find_filter(name);
    if (!filter) {
        blog(LOG_WARNING, "[Move Workflow Tests] Filter not found: %s", name);
        return;
    }

    blog(LOG_INFO, "[Move Workflow Tests] %s -> %s", label, name);
    obs_source_set_enabled(filter, false);
    obs_source_set_enabled(filter, true);
    obs_source_release(filter);
}

void trigger_director_menu(const char *text)
{
    auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!mainWindow) {
        blog(LOG_WARNING, "[Move Workflow Tests] OBS main window unavailable");
        return;
    }

    const auto actions = mainWindow->findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action && action->text() == QString::fromUtf8(text)) {
            blog(LOG_INFO, "[Move Workflow Tests] Triggering director test: %s", text);
            action->trigger();
            return;
        }
    }

    blog(LOG_WARNING, "[Move Workflow Tests] Director test action not found: %s", text);
}

void duration_test()
{
    trigger_director_menu("Move Workflow: Test Duration Override (Left)");
}

void multiple_end_actions()
{
    trigger_director_menu("Move Workflow: Test Multiple End Actions (Top Left -> Bottom Center + Bottom Right)");
}

void end_actions_with_delay()
{
    trigger_director_menu("Move Workflow: Test End Actions with Start Delay (Bottom Right +1000ms)");
}

void start_trigger_test()
{
    trigger_director_menu("Move Workflow: Test Left");
}

class TestWindow final : public QDialog {
public:
    explicit TestWindow(QWidget *parent) : QDialog(parent)
    {
        setWindowTitle("Move Workflow Tests");
        setMinimumWidth(430);

        auto *layout = new QVBoxLayout(this);
        auto *info = new QLabel(
            "Movement tests use the existing Move filters. Director tests invoke the director test actions.", this);
        info->setWordWrap(true);
        layout->addWidget(info);

        auto *top = new QGroupBox("Top graphic", this);
        auto *topLayout = new QHBoxLayout(top);
        addButton(topLayout, "Left", [] { trigger_filter("Move Source - Top - Left", "Top Left"); });
        addButton(topLayout, "Center", [] { trigger_filter("Move Source - Top - Center", "Top Center"); });
        addButton(topLayout, "Right", [] { trigger_filter("Move Source - Top - Right", "Top Right"); });
        layout->addWidget(top);

        auto *bottom = new QGroupBox("Bottom graphic", this);
        auto *bottomLayout = new QHBoxLayout(bottom);
        addButton(bottomLayout, "Left", [] { trigger_filter("Move Source - Bottom - Left", "Bottom Left"); });
        addButton(bottomLayout, "Center", [] { trigger_filter("Move Source - Bottom - Center", "Bottom Center"); });
        addButton(bottomLayout, "Right", [] { trigger_filter("Move Source - Bottom - Right", "Bottom Right"); });
        layout->addWidget(bottom);

        auto *director = new QGroupBox("Director tests", this);
        auto *directorLayout = new QVBoxLayout(director);
        addButton(directorLayout, "Duration Override — Top Left → 1000ms", [] { duration_test(); });
        addButton(directorLayout, "Multiple End Actions — Bottom Center + Bottom Right", [] { multiple_end_actions(); });
        addButton(directorLayout, "End Actions + Start Delay — Bottom Right +1000ms", [] { end_actions_with_delay(); });
        addButton(directorLayout, "Start Trigger — Enable", [] { start_trigger_test(); });
        layout->addWidget(director);

        auto *close = new QPushButton("Close", this);
        connect(close, &QPushButton::clicked, this, &QDialog::close);
        layout->addWidget(close);
    }

private:
    template<typename Layout, typename Function>
    static void addButton(Layout *layout, const QString &text, Function function)
    {
        auto *button = new QPushButton(text);
        QObject::connect(button, &QPushButton::clicked, function);
        layout->addWidget(button);
    }
};

QPointer<TestWindow> window;

void show_tests()
{
    if (!window) {
        auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        window = new TestWindow(mainWindow);
        window->setAttribute(Qt::WA_DeleteOnClose);
    }
    window->show();
    window->raise();
    window->activateWindow();
}

void hide_legacy_test_menu_items()
{
    auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!mainWindow)
        return;

    const QStringList legacyNames = {
        "Move Workflow: Test Left",
        "Move Workflow: Test Center",
        "Move Workflow: Test Right",
        "Move Workflow: Test Duration Override (Left)",
        "Move Workflow: Test End Action Left -> Center",
        "Move Workflow: Test Multiple End Actions (Top Left -> Bottom Center + Bottom Right)",
        "Move Workflow: Test End Actions with Start Delay (Bottom Right +1000ms)",
    };

    const auto actions = mainWindow->findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action && legacyNames.contains(action->text())) {
            action->setVisible(false);
            blog(LOG_INFO, "[Move Workflow Tests] Hidden legacy Tools menu item: %s",
                 action->text().toUtf8().constData());
        }
    }
}

void register_menu()
{
    hide_legacy_test_menu_items();

    QAction *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction("Move Workflow Tests"));
    if (!action)
        return;
    QObject::connect(action, &QAction::triggered, [] { show_tests(); });
}

struct AutoRegister {
    AutoRegister()
    {
        QTimer::singleShot(0, register_menu);
        QTimer::singleShot(250, hide_legacy_test_menu_items);
    }
};

AutoRegister auto_register;

} // namespace

void move_workflow_register_tests(void)
{
    QTimer::singleShot(0, register_menu);
}
