#pragma once

#include "workflow-model.h"

class QUndoStack;

class WorkflowUndo final {
public:
    WorkflowUndo();
    ~WorkflowUndo();
    void reset(workflow_t *workflow);
    void capture();
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;

private:
    workflow_t *workflow_ = nullptr;
    workflow_t last_{};
    QUndoStack *stack_ = nullptr;
};
