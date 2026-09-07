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
    Panel(QWidget *parent, workflow_engine *engine)
        : QWidget(parent), engine_(engine), rows_(new QVBoxLayout), timer_(new QTimer(this))
    {
        auto *layout = new QVBoxLayout(this);
        auto *header = new QHBoxLayout;
        header->addWidget(new QLabel("Active Runs", this));
        stopAll_ = new QPushButton("Stop Workflow", this);
        header->addWidget(stopAll_);
        layout->addLayout(header);
        layout->addLayout(rows_);
        layout->addStretch();
        connect(stopAll_, &QPushButton::clicked, this, [this] {
            workflow_engine_service_stop_workflow(workflowId_.toUtf8().constData());
            rebuild();
        });
        connect(timer_, &QTimer::timeout, this, [this] { rebuild(); });
        timer_->start(250);
        rebuild();
    }
    void setWorkflow(const QString &id) { workflowId_ = id; rebuild(); }
    void refreshRuns() { rebuild(); }
private:
    void rebuild()
    {
        while (auto *item = rows_->takeAt(0)) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        const auto runs = workflow_engine_monitor::active_runs(engine_, workflowId_);
        stopAll_->setEnabled(!workflowId_.isEmpty() && !runs.isEmpty());
        for (const auto &run : runs) {
            auto *row = new QWidget(this);
            auto *layout = new QHBoxLayout(row);
            QString text = QString("Run %1  %2").arg(run.run_id)
                .arg(run.current_node_id.isEmpty() ? "Starting" : run.current_node_id);
            if (run.waiting_for_shortcut) text += "  [Waiting for shortcut]";
            layout->addWidget(new QLabel(text, row));
            auto *stop = new QPushButton("Stop", row);
            connect(stop, &QPushButton::clicked, this, [this, id = run.run_id] {
                workflow_engine_service_stop_run(workflowId_.toUtf8().constData(), id);
                rebuild();
            });
            layout->addWidget(stop);
            rows_->addWidget(row);
        }
    }
    workflow_engine *engine_ = nullptr;
    QVBoxLayout *rows_;
    QPushButton *stopAll_ = nullptr;
    QTimer *timer_;
    QString workflowId_;
};
}

namespace workflow_editor_active_runs {
QWidget *create(QWidget *parent, workflow_engine *engine) { return new Panel(parent, engine); }
void set_workflow(QWidget *panel, const QString &workflowId)
{ if (auto *p = dynamic_cast<Panel *>(panel)) p->setWorkflow(workflowId); }
void refresh(QWidget *panel)
{ if (auto *p = dynamic_cast<Panel *>(panel)) p->refreshRuns(); }
}
