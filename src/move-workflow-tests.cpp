#include "move-workflow-tests.h"

#include <obs.h>
#include <obs-frontend-api.h>

#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPointer>
#include <QMainWindow>

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

void duration_test()
{
    obs_source_t *filter = find_filter("Move Source - Top - Left");
    if (!filter)
        return;

    obs_data_t *settings = obs_source_get_settings(filter);
    if (!settings) {
        obs_source_release(filter);
        return;
    }

    const bool old_custom = obs_data_get_bool(settings, "custom_duration");
    const int64_t old_duration = obs_data_get_int(settings, "duration");
    obs_data_set_bool(settings, "custom_duration", true);
    obs_data_set_int(settings, "duration", 1000);
    obs_source_update(filter, settings);
    obs_data_release(settings);

    blog(LOG_INFO, "[Move Workflow Tests] Duration override -> Top Left = 1000ms");
    obs_source_set_enabled(filter, false);
    obs_source_set_enabled(filter, true);

    QPointer<QObject> guard = new QObject();
    QTimer::singleShot(1200, guard, [filter, old_custom, old_duration, guard]() {
        obs_data_t *restore = obs_source_get_settings(filter);
        if (restore) {
            obs_data_set_bool(restore, "custom_duration", old_custom);
            obs_data_set_int(restore, "duration", old_duration);
            obs_source_update(filter, restore);
            obs_data_release(restore);
        }
        obs_source_release(filter);
        delete guard;
        blog(LOG_INFO, "[Move Workflow Tests] Duration override restored");
    });
}

void multiple_end_actions()
{
    blog(LOG_INFO, "[Move Workflow Tests] Multiple End Actions: Bottom Center + Bottom Right (parallel)");
    trigger_filter("Move Source - Top - Left", "Parent action");
    QTimer::singleShot(350, [] {
        trigger_filter("Move Source - Bottom - Center", "End Action 1");
        trigger_filter("Move Source - Bottom - Right", "End Action 2");
    });
}

void end_actions_with_delay()
{
    blog(LOG_INFO, "[Move Workflow Tests] End Actions: Bottom Center immediate, Bottom Right +1000ms");
    trigger_filter("Move Source - Top - Left", "Parent action");
    QTimer::singleShot(350, [] {
        trigger_filter("Move Source - Bottom - Center", "End Action 1");
        QTimer::singleShot(1000, [] {
            trigger_filter("Move Source - Bottom - Right", "End Action 2 (+1000ms)");
        });
    });
}

class TestWindow final : public QDialog {
public:
    explicit TestWindow(QWidget *parent) : QDialog(parent)
    {
        setWindowTitle("Move Workflow Tests");
        setMinimumWidth(430);

        auto *layout = new QVBoxLayout(this);
        auto *info = new QLabel(
            "These tests exercise the existing Move filters through the director test setup.", this);
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
        addButton(directorLayout, "Start Trigger — Enable", [] {
            blog(LOG_INFO, "[Move Workflow Tests] Start Trigger override -> Enable");
            trigger_filter("Move Source - Top - Left", "Start Trigger Enable");
        });
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

void register_menu()
{
    QAction *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction("Move Workflow Tests"));
    if (!action)
        return;
    QObject::connect(action, &QAction::triggered, [] { show_tests(); });
}

struct AutoRegister {
    AutoRegister()
    {
        QTimer::singleShot(0, register_menu);
    }
};

AutoRegister auto_register;

} // namespace

void move_workflow_register_tests(void)
{
    QTimer::singleShot(0, register_menu);
}
