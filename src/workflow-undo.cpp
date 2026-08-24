#include "workflow-undo.h"
#include <QUndoCommand>
#include <QUndoStack>
#include <obs-module.h>
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

static void log_stack(const char *tag, const QUndoStack *stack)
{
    blog(LOG_INFO, "[Move Workflow] %s: count=%d index=%d canUndo=%d canRedo=%d",
         tag, stack->count(), stack->index(), stack->canUndo(), stack->canRedo());
}
}

WorkflowUndo::WorkflowUndo() : stack_(new QUndoStack) {}
WorkflowUndo::~WorkflowUndo() { delete stack_; }
void WorkflowUndo::reset(workflow_t *workflow, workflow_manager_t *manager)
{
    workflow_ = workflow; manager_ = manager; stack_->clear(); suppressCapture_ = false;
    if (workflow_) last_ = *workflow_; else last_ = {};
    if (manager_) last_manager_ = std::make_unique<workflow_manager_t>(*manager_);
    else last_manager_.reset();
    log_stack("RESET", stack_);
}
void WorkflowUndo::capture()
{
    if (!workflow_) return;
    log_stack("CAPTURE before", stack_);
    if (suppressCapture_) {
        if (std::memcmp(&replayState_, workflow_, sizeof(workflow_t)) == 0) {
            blog(LOG_INFO, "[Move Workflow] CAPTURE suppressed: replay state matches");
            return;
        }
        suppressCapture_ = false;
        blog(LOG_INFO, "[Move Workflow] CAPTURE suppression cleared: workflow changed");
    }
    if (std::memcmp(&last_, workflow_, sizeof(workflow_t)) == 0) {
        blog(LOG_INFO, "[Move Workflow] CAPTURE skipped: snapshot unchanged");
        return;
    }
    auto before = std::make_unique<workflow_t>(last_);
    auto after = std::make_unique<workflow_t>(*workflow_);
    log_stack("CAPTURE push", stack_);
    stack_->push(new WorkflowSnapshotCommand(workflow_, std::move(before), std::move(after)));
    last_ = *workflow_;
    log_stack("CAPTURE after push", stack_);
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
    log_stack("CAPTURE manager", stack_);
}
bool WorkflowUndo::undo()
{
    log_stack("UNDO before", stack_);
    if (!stack_->canUndo()) return false;
    stack_->undo();
    replayState_ = workflow_ ? *workflow_ : workflow_t{};
    suppressCapture_ = true;
    log_stack("UNDO after", stack_);
    workflow_ = manager_ ? workflow_manager_selected(manager_) : workflow_;
    if (workflow_) last_ = *workflow_;
    if (manager_) last_manager_ = std::make_unique<workflow_manager_t>(*manager_);
    return true;
}
bool WorkflowUndo::redo()
{
    log_stack("REDO before", stack_);
    if (!stack_->canRedo()) return false;
    stack_->redo();
    replayState_ = workflow_ ? *workflow_ : workflow_t{};
    suppressCapture_ = true;
    log_stack("REDO after", stack_);
    workflow_ = manager_ ? workflow_manager_selected(manager_) : workflow_;
    if (workflow_) last_ = *workflow_;
    if (manager_) last_manager_ = std::make_unique<workflow_manager_t>(*manager_);
    return true;
}
bool WorkflowUndo::canUndo() const { return stack_->canUndo(); }
bool WorkflowUndo::canRedo() const { return stack_->canRedo(); }
