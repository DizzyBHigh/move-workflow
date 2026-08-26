#pragma once

class NodeItem;
class QWidget;

QWidget *create_workflow_editor_properties(QWidget *parent, void (*edit_node)(NodeItem *));
void workflow_editor_properties_set_node(QWidget *properties, NodeItem *node);
