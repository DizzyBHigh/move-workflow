#include "workflow-debug-ui.h"
#include "workflow-debug.h"

#include <QCheckBox>

QCheckBox *workflow_debug_create_control(QWidget *parent)
{
    auto *check = new QCheckBox("Debug logging", parent);
    check->setToolTip("Enable detailed Move Workflow execution logging in the OBS log.");
    check->setChecked(workflow_debug_is_enabled());
    QObject::connect(check, &QCheckBox::toggled, [](bool enabled) {
        workflow_debug_set_enabled(enabled);
    });
    return check;
}
