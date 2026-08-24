#include "workflow-undo.h"
#include <QUndoCommand>
#include <QUndoStack>
#include <QDebug>
#include <cstring>
#include <memory>

namespace {
class WorkflowSnapshotCommand final : public QUndoCommand {
public:
    WorkflowSnapshotCommand(workflow_t *workflow, std::unique_ptr<workflow_t> before,
                            std::unique_ptr<workflow_t> after)
        : workflow_(workflow), before_(std::move(before)), after_(std::move(after))
    { setText("Workflow change"); }
    void undo() override { *workflow_ = *before_; }
    void redo() override { *workflow_ = *after_; }
private:
    workflow_t *workflow_;
    std::unique_ptr<workflow_t> before_, after_;
};

class ManagerSnapshotCommand final : public QUndoCommand {
public:
    ManagerSnapshotCommand(workflow_manager_t *manager,
                           std::unique_ptr<workflow_manager_t> before,
                           std::unique_ptr<workflow_manager_t> after)
        : manager_(manager), before_(std::move(before)), after_(std::move(after))
    { setText("Delete workflow"); }
    void undo() override { *manager_ = *before_; }
    void redo() override { *manager_ = *after_; }
private:
    workflow_manager_t *manager_;
    std::unique_ptr<workflow_manager_t> before_, after_;
};
}

WorkflowUndo::WorkflowUndo() : stack_(new QUndoStack) {}
WorkflowUndo::~WorkflowUndo() { delete stack_; }
void WorkflowUndo::reset(workflow_t *workflow, workflow_manager_t *manager)
{
    workflow_ = workflow; manager_ = manager; stack_->clear(); suppressCapture_ = false;
    if (workflow_) last_ = *workflow_; else last_ = {};
    if (manager_) last_manager_ = std::make_unique<workflow_manager_t>(*manager_);
    else last_manager_.reset();
}
void WorkflowUndo::capture()
{
    if (!workflow_) return;
    if (suppressCapture_) {
        suppressCapture_ = false;
        last_ = *workflow_;
        return;
    }
    if (std::memcmp(&last_, workflow_, sizeof(workflow_t)) == 0) return;
    auto before = std::make_unique<workflow_t>(last_);
    auto after = std::make_unique<workflow_t>(*workflow_);
    stack_->push(new WorkflowSnapshotCommand(workflow_, std::move(before), std::move(after)));
    last_ = *workflow_;
}
void WorkflowUndo::prepareManagerCapture()
{
    if (manager_) last_manager_ = std::make_unique<workflow_manager_t>(*manager_);
}
void WorkflowUndo::captureManager()
{
    if (!manager_) return;
    if (last_manager_ && std::memcmp(last_manager_.get(), manager_, sizeof(workflow_manager_t)) == 0) return;
    auto before = last_manager_ ? std::move(last_manager_) : std::make_unique<workflow_manager_t>();
    auto after = std::make_unique<workflow_manager_t>(*manager_);
    stack_->push(new ManagerSnapshotCommand(manager_, std::move(before), std::move(after)));
    last_manager_ = std::make_unique<workflow_manager_t>(*manager_);
    workflow_ = workflow_manager_selected(manager_);
    if (workflow_) last_ = *workflow_;
}
bool WorkflowUndo::undo()
{
    qDebug() << "[Move Workflow] UNDO requested: canUndo=" << stack_->canUndo()
             << "canRedo=" << stack_->canRedo() << "index=" << stack_->index()
             << "count=" << stack_->count();
    if (!stack_->canUndo()) return false;
    stack_->undo();
    suppressCapture_ = true;
    qDebug() << "[Move Workflow] UNDO complete: canUndo=" << stack_->canUndo()
             << "canRedo=" << stack_->canRedo() << "index=" << stack_->index()
             << "count=" << stack_->count();
    workflow_ = manager_ ? workflow_manager_selected(manager_) : workflow_;
    if (workflow_) last_ = *workflow_;
    if (manager_) last_manager_ = std::make_unique<workflow_manager_t>(*manager_);
    return true;
}
bool WorkflowUndo::redo()
{
    qDebug() << "[Move Workflow] REDO requested: canUndo=" << stack_->canUndo()
             << "canRedo=" << stack_->canRedo() << "index=" << stack_->index()
             << "count=" << stack_->count();
    if (!stack_->canRedo()) return false;
    stack_->redo();
    suppressCapture_ = true;
    qDebug() << "[Move Workflow] REDO complete: canUndo=" << stack_->canUndo()
             << "canRedo=" << stack_->canRedo() << "index=" << stack_->index()
             << "count=" << stack_->count();
    workflow_ = manager_ ? workflow_manager_selected(manager_) : workflow_;
    if (workflow_) last_ = *workflow_;
    if (manager_) last_manager_ = std::make_unique<workflow_manager_t>(*manager_);
    return true;
}
bool WorkflowUndo::canUndo() const { return stack_->canUndo(); }
bool WorkflowUndo::canRedo() const { return stack_->canRedo(); }
