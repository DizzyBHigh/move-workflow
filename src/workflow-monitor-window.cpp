#include "workflow-monitor-window.hpp"
#include "workflow-editor-active-runs.hpp"
#include "workflow-engine-service.h"
#include "workflow-persistence.h"

#include <obs-frontend-api.h>
#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr const char *DOCK_ID = "move_workflow_monitor";
QPointer<QDockWidget> monitorDock;

class MonitorDock : public QDockWidget {
public:
    explicit MonitorDock(QWidget *parent) : QDockWidget("Move Workflow Monitor", parent)
    {
        setObjectName(DOCK_ID);
        setAllowedAreas(Qt::AllDockWidgetAreas);
        auto *content = new QWidget(this);
        auto *layout = new QVBoxLayout(content);
        auto *header = new QHBoxLayout;
        header->addWidget(new QLabel("Workflow:", content));
        workflows_ = new QComboBox(content);
        header->addWidget(workflows_, 1);
        auto *refresh = new QPushButton("Refresh", content);
        header->addWidget(refresh);
        layout->addLayout(header);
        activeRuns_ = workflow_editor_active_runs::create(content, workflow_engine_service_engine());
        layout->addWidget(activeRuns_, 1);
        setWidget(content);
        connect(workflows_, &QComboBox::currentIndexChanged, this, [this] { updateWorkflow(); });
        connect(refresh, &QPushButton::clicked, this, [this] { reloadWorkflows(); });
        reloadWorkflows();
    }
private:
    void reloadWorkflows()
    {
        const QString previous = workflows_->currentData().toString();
        workflows_->blockSignals(true); workflows_->clear();
        workflows_->addItem("All Workflows", QString());
        auto *manager = workflow_persistence_manager();
        if (manager) for (size_t i = 0; i < workflow_manager_count(manager); ++i) {
            const auto *workflow = workflow_manager_at_const(manager, i);
            if (workflow) workflows_->addItem(QString::fromUtf8(workflow->name), QString::fromUtf8(workflow->id));
        }
        const int index = workflows_->findData(previous);
        workflows_->setCurrentIndex(index >= 0 ? index : 0);
        workflows_->blockSignals(false); updateWorkflow();
    }
    void updateWorkflow()
    { workflow_editor_active_runs::set_workflow(activeRuns_, workflows_->currentData().toString()); }
    QComboBox *workflows_ = nullptr;
    QWidget *activeRuns_ = nullptr;
};
}

namespace workflow_monitor_window {
QWidget *show(QWidget *parent)
{
    if (!monitorDock) {
        QWidget *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
        auto *dock = new MonitorDock(mainWindow ? mainWindow : parent);
        if (!obs_frontend_add_custom_qdock(DOCK_ID, dock)) {
            delete dock;
            return nullptr;
        }
        monitorDock = dock;
    }
    monitorDock->show(); monitorDock->raise();
    return monitorDock;
}
void close()
{
    if (!monitorDock) return;
    obs_frontend_remove_dock(DOCK_ID);
    monitorDock = nullptr;
}
}
