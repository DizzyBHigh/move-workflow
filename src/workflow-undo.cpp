#include "workflow-undo.h"
#include <QUndoCommand>
#include <QUndoStack>
#include <cstring>

namespace {
class SnapshotCommand final : public QUndoCommand {
public:
    SnapshotCommand(workflow_t *workflow, const workflow_t &before, const workflow_t &after)
        : workflow_(workflow), before_(before), after_(after) { setText("Workflow change"); }
    void undo() override { *workflow_ = before_; }
    void redo() override { *workflow_ = after_; }
private:
    workflow_t *workflow_;
    workflow_t before_{};
    workflow_t after_{};
};
}

WorkflowUndo::WorkflowUndo() : stack_(new QUndoStack) {}
WorkflowUndo::~WorkflowUndo() { delete stack_; }
void WorkflowUndo::reset(workflow_t *workflow)
{
    workflow_ = workflow; stack_->clear();
    if (workflow_) last_ = *workflow_; else std::memset(&last_, 0, sizeof(last_));
}
void WorkflowUndo::capture()
{
    if (!workflow_ || std::memcmp(&last_, workflow_, sizeof(last_)) == 0) return;
    const workflow_t before = last_; const workflow_t after = *workflow_;
    stack_->push(new SnapshotCommand(workflow_, before, after)); last_ = after;
}
bool WorkflowUndo::undo()
{
    if (!stack_->canUndo()) return false; stack_->undo(); last_ = *workflow_; return true;
}
bool WorkflowUndo::redo()
{
    if (!stack_->canRedo()) return false; stack_->redo(); last_ = *workflow_; return true;
}
bool WorkflowUndo::canUndo() const { return stack_->canUndo(); }
bool WorkflowUndo::canRedo() const { return stack_->canRedo(); }
