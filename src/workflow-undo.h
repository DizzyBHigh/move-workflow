#pragma once

#include "workflow-manager.h"
#include <memory>

class QUndoStack;

class WorkflowUndo final {
public:
    WorkflowUndo();
    ~WorkflowUndo();
    void reset(workflow_t *workflow, workflow_manager_t *manager);
    void capture();
    void prepareManagerCapture();
    void captureManager();
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;

private:
    workflow_t *workflow_ = nullptr;
    workflow_manager_t *manager_ = nullptr;
    workflow_t last_{};
    std::unique_ptr<workflow_manager_t> last_manager_;
    QUndoStack *stack_ = nullptr;
    bool suppressCapture_ = false;
};
