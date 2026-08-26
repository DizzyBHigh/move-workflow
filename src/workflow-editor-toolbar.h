#pragma once

#include <functional>

class QWidget;

struct workflow_editor_toolbar_callbacks {
    std::function<void()> add;
    std::function<void()> edit;
    std::function<void()> copy;
    std::function<void()> paste;
    std::function<void()> duplicate;
    std::function<void()> remove;
    std::function<void()> test;
    std::function<void()> zoom_out;
    std::function<void()> zoom_reset;
    std::function<void()> zoom_in;
    std::function<void()> fit;
    std::function<void()> close;
};

QWidget *create_workflow_editor_toolbar(
    QWidget *parent,
    workflow_editor_toolbar_callbacks callbacks);

void workflow_editor_toolbar_set_selection_state(
    QWidget *toolbar, bool has_selection, bool can_paste);
