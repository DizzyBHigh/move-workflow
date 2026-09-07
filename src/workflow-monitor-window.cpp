#include "workflow-monitor-window.hpp"
#include "workflow-editor-active-runs.hpp"
#include "workflow-engine-service.h"
#include "workflow-persistence.h"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
QPointer<QDialog> monitorWindow;

class MonitorWindow : public QDialog {
public:
    explicit MonitorWindow(QWidget *parent) : QDialog(parent)
    {
        setWindowTitle("Move Workflow Monitor");
        resize(600, 400);
        auto *layout = new QVBoxLayout(this);
        auto *header = new QHBoxLayout;
        header->addWidget(new QLabel("Workflow:", this));
        workflows_ = new QComboBox(this);
        header->addWidget(workflows_, 1);
        auto *refresh = new QPushButton("Refresh", this);
        header->addWidget(refresh);
        layout->addLayout(header);
        activeRuns_ = workflow_editor_active_runs::create(
            this, workflow_engine_service_engine());
        layout->addWidget(activeRuns_, 1);
        connect(workflows_, &QComboBox::currentIndexChanged,
                this, [this] { updateWorkflow(); });
        connect(refresh, &QPushButton::clicked,
                this, [this] { reloadWorkflows(); });
        reloadWorkflows();
    }
private:
    void reloadWorkflows()
    {
        const QString previous = workflows_->currentData().toString();
        workflows_->blockSignals(true);
        workflows_->clear();
        workflows_->addItem("All Workflows", QString());
        auto *manager = workflow_persistence_manager();
        if (manager) {
            const size_t count = workflow_manager_count(manager);
            for (size_t i = 0; i < count; ++i) {
                const auto *workflow = workflow_manager_at_const(manager, i);
                if (!workflow) continue;
                workflows_->addItem(QString::fromUtf8(workflow->name),
                                    QString::fromUtf8(workflow->id));
            }
        }
        const int index = workflows_->findData(previous);
        workflows_->setCurrentIndex(index >= 0 ? index : 0);
        workflows_->blockSignals(false);
        updateWorkflow();
    }
    void updateWorkflow()
    {
        workflow_editor_active_runs::set_workflow(
            activeRuns_, workflows_->currentData().toString());
    }
    QComboBox *workflows_ = nullptr;
    QWidget *activeRuns_ = nullptr;
};
}

namespace workflow_monitor_window {
QWidget *show(QWidget *parent)
{
    if (!monitorWindow)
        monitorWindow = new MonitorWindow(parent);
    monitorWindow->show();
    monitorWindow->raise();
    monitorWindow->activateWindow();
    return monitorWindow;
}
void close()
{
    if (!monitorWindow) return;
    monitorWindow->close();
    monitorWindow->deleteLater();
    monitorWindow = nullptr;
}
}
