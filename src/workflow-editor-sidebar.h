#pragma once

#include <functional>
#include <QList>

class NodeItem;
class QWidget;

struct workflow_editor_sidebar_callbacks {
    std::function<void()> add_trigger;
    std::function<void(const char *)> add_node;
    std::function<void(const char *)> select_node;
    std::function<void()> edit_node;
    std::function<void()> copy_node;
    std::function<void()> paste_node;
    std::function<void()> duplicate_node;
    std::function<void()> delete_node;
};

QWidget *create_workflow_editor_sidebar(QWidget *parent,
                                        workflow_editor_sidebar_callbacks callbacks);
void workflow_editor_sidebar_set_selection_state(QWidget *sidebar, bool has_selection, bool can_paste);
void workflow_editor_sidebar_set_workflow_nodes(QWidget *sidebar, const QList<NodeItem *> &nodes, NodeItem *selected);
