#include "move-workflow-editor-panel-ui.hpp"

#include <QApplication>
#include <QDialog>
#include <QTimer>
#include <QWidget>

namespace {

class WorkflowPanelInstaller final : public QObject {
public:
    WorkflowPanelInstaller()
    {
        timer_.setInterval(400);
        connect(&timer_, &QTimer::timeout, this, &WorkflowPanelInstaller::installOnOpenEditors);
        timer_.start();
        QTimer::singleShot(0, this, &WorkflowPanelInstaller::installOnOpenEditors);
    }

private:
    void installOnOpenEditors()
    {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            auto *dialog = qobject_cast<QDialog *>(widget);
            if (!dialog || dialog->windowTitle() != "Move Workflow Editor")
                continue;
            if (dialog->property("moveWorkflowSidePanelInstalled").toBool())
                continue;
            move_workflow_editor_panel_ui::install(dialog);
        }
    }

    QTimer timer_;
};

WorkflowPanelInstaller installer;

} // namespace
