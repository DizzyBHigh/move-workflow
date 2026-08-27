#pragma once

#include <functional>

class NodeItem;
class QWidget;

QWidget *create_workflow_editor_properties(QWidget *parent, std::function<void(NodeItem *)> edit_node);
void workflow_editor_properties_set_node(QWidget *properties, NodeItem *node);
