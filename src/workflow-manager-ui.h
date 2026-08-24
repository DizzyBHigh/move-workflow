#pragma once

class QWidget;
class QComboBox;
class QCheckBox;

struct workflow_manager;

QWidget *create_workflow_manager_ui(workflow_manager *manager, QWidget *parent = nullptr);
