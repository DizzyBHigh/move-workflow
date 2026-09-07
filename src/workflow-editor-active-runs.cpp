#include "workflow-editor-active-runs.hpp"
#include "workflow-engine-monitor.hpp"
#include "workflow-engine-service.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {
class Panel : public QWidget {
public:
    Panel(QWidget *parent, workflow_engine *)
        : QWidget(parent), rows_(new QVBoxLayout), timer_(new QTimer(this))
    {
        auto *layout = new QVBoxLayout(this); auto *header = new QHBoxLayout;
        header->addWidget(new QLabel("Active Runs", this));
        stopAll_ = new QPushButton("Stop All", this); header->addWidget(stopAll_);
        layout->addLayout(header); layout->addLayout(rows_); layout->addStretch();
        connect(stopAll_, &QPushButton::clicked, this, [this] {
            workflow_engine_monitor::stop_scope(workflow_engine_service_engine(), workflowId_);
            rebuild(); notifyRefresh();
        });
        connect(timer_, &QTimer::timeout, this, [this] { rebuild(); notifyRefresh(); });
        timer_->start(250); rebuild();
    }
    void setWorkflow(const QString &id) { workflowId_ = id; rebuild(); }
    void refreshRuns() { rebuild(); }
    void setRefreshCallback(std::function<void()> callback) { refreshCallback_ = std::move(callback); }
private:
    void notifyRefresh() { if (refreshCallback_) refreshCallback_(); }
    void rebuild()
    {
        while (auto *item = rows_->takeAt(0)) { if (item->widget()) item->widget()->deleteLater(); delete item; }
        auto *engine = workflow_engine_service_engine();
        const auto runs = workflow_engine_monitor::active_runs(engine, workflowId_);
        stopAll_->setEnabled(!workflowId_.isEmpty() && !runs.isEmpty());
        for (const auto &run : runs) {
            auto *row = new QWidget(this); auto *layout = new QVBoxLayout(row);
            auto *top = new QHBoxLayout;
            top->addWidget(new QLabel(QString("Run %1    %2").arg(run.run_id).arg(
                run.workflow_name.isEmpty() ? "Unnamed Workflow" : run.workflow_name), row), 1);
            auto *stop = new QPushButton("Stop Instance", row);
            const QString runWorkflow = run.workflow_id;
            connect(stop, &QPushButton::clicked, this, [this, runWorkflow, id = run.run_id] {
                workflow_engine_service_stop_run(runWorkflow.toUtf8().constData(), id);
                rebuild(); notifyRefresh();
            });
            top->addWidget(stop); layout->addLayout(top);
            const QString node = run.current_node_name.isEmpty() ? "Starting" : run.current_node_name;
            QString status = QString("Status: %1").arg(node);
            if (run.waiting_for_shortcut) {
                status += " [Waiting for shortcut]";
            } else if (!run.phase.isEmpty() && run.duration_ms > 0) {
                const double elapsed = run.elapsed_ms / 1000.0;
                const double duration = run.duration_ms / 1000.0;
                status += QString("    %1: %2 / %3 s").arg(run.phase)
                    .arg(elapsed, 0, 'f', 2).arg(duration, 0, 'f', 2);
            }
            layout->addWidget(new QLabel(status, row));
            rows_->addWidget(row);
        }
    }
    QVBoxLayout *rows_; QPushButton *stopAll_ = nullptr;
    QTimer *timer_; QString workflowId_; std::function<void()> refreshCallback_;
};
}

namespace workflow_editor_active_runs {
QWidget *create(QWidget *parent, workflow_engine *engine) { return new Panel(parent, engine); }
void set_workflow(QWidget *panel, const QString &workflowId)
{ if (auto *p = dynamic_cast<Panel *>(panel)) p->setWorkflow(workflowId); }
void refresh(QWidget *panel) { if (auto *p = dynamic_cast<Panel *>(panel)) p->refreshRuns(); }
void set_refresh_callback(QWidget *panel, std::function<void()> callback)
{ if (auto *p = dynamic_cast<Panel *>(panel)) p->setRefreshCallback(std::move(callback)); }
}
