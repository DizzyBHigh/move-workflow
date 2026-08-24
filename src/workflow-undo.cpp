#include "workflow-undo.h"
#include <QUndoCommand>
#include <QUndoStack>
#include <cstring>
#include <memory>

namespace {
class SnapshotCommand final : public QUndoCommand {
public:
    SnapshotCommand(workflow_t *workflow, std::unique_ptr<workflow_t> before,
                    std::unique_ptr<workflow_t> after)
        : workflow_(workflow), before_(std::move(before)), after_(std::move(after))
    { setText("Workflow change"); }
    void undo() override { *workflow_ = *before_; }
    void redo() override { *workflow_ = *after_; }
private:
    workflow_t *workflow_;
    std::unique_ptr<workflow_t> before_;
    std::unique_ptr<workflow_t> after_;
};
}

WorkflowUndo::WorkflowUndo() : stack_(new QUndoStack) {}
WorkflowUndo::~WorkflowUndo() { delete stack_; }
void WorkflowUndo::reset(workflow_t *workflow)
{
    workflow_ = workflow; stack_->clear();
    if (workflow_) last_ = std::make_unique<workflow_t>(*workflow_);
    else last_.reset();
}
void WorkflowUndo::capture()
{
    if (!workflow_) return;
    if (last_ && std::memcmp(last_.get(), workflow_, sizeof(workflow_t)) == 0) return;
    auto before = last_ ? std::make_unique<workflow_t>(*last_) : std::make_unique<workflow_t>();
    auto after = std::make_unique<workflow_t>(*workflow_);
    stack_->push(new SnapshotCommand(workflow_, std::move(before), std::move(after)));
    last_ = std::make_unique<workflow_t>(*workflow_);
}
bool WorkflowUndo::undo()
{
    if (!stack_->canUndo()) return false; stack_->undo(); last_ = std::make_unique<workflow_t>(*workflow_); return true;
}
bool WorkflowUndo::redo()
{
    if (!stack_->canRedo()) return false; stack_->redo(); last_ = std::make_unique<workflow_t>(*workflow_); return true;
}
bool WorkflowUndo::canUndo() const { return stack_->canUndo(); }
bool WorkflowUndo::canRedo() const { return stack_->canRedo(); }
