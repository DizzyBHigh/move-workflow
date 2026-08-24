#pragma once

#include "workflow-node.h"

#include <QList>

class QWidget;

bool edit_node_settings(NodeItem *node,
                        const QList<NodeItem *> &nodes,
                        QWidget *parent = nullptr);
