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
    explicit Panel(QWidget *parent) : QWidget(parent), rows_(new QVBoxLayout), timer_(new QTimer(this))
    {
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Active Runs", this));
        layout->addLayout(rows_);
        layout->addStretch();
        connect(timer_, &QTimer::timeout, this, [this] { rebuild(); });
        timer_->start(250);
    }
    void setWorkflow(const QString &id) { workflowId_ = id; rebuild(); }
private:
    void rebuild()
    {
        while (auto *item = rows_->takeAt(0)) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        for (const auto &run : workflow_engine_monitor::active_runs(runs_, workflowId_)) {
            auto *row = new QWidget(this);
            auto *layout = new QHBoxLayout(row);
            QString text = QString("Run %1  %2")
                .arg(run.run_id).arg(run.current_node_id.isEmpty() ? "Starting" : run.current_node_id);
            if (run.waiting_for_shortcut) text += "  [Waiting for shortcut]";
            layout->addWidget(new QLabel(text, row));
            auto *stop = new QPushButton("Stop", row);
            connect(stop, &QPushButton::clicked, this, [this, id = run.run_id] {
                workflow_engine_monitor::stop_run(runs_, id);
                rebuild();
            });
            layout->addWidget(stop);
            rows_->addWidget(row);
        }
    }
    QVBoxLayout *rows_;
    QTimer *timer_;
    QString workflowId_;
    workflow_engine_runs_t *runs_ = nullptr;
};
}

namespace workflow_editor_active_runs {
QWidget *create(QWidget *parent) { return new Panel(parent); }
void set_workflow(QWidget *panel, const QString &workflowId)
{ if (auto *p = dynamic_cast<Panel *>(panel)) p->setWorkflow(workflowId); }
void refresh(QWidget *panel)
{ if (auto *p = dynamic_cast<Panel *>(panel)) p->update(); }
}
