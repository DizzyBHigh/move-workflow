#include "workflow-editor-sidebar-play.hpp"
#include "workflow-engine-service.h"

#include <QListWidget>
#include <QPushButton>
#include <QWidget>

void workflow_editor_sidebar_install_play_buttons(
    QWidget *sidebar, std::function<const char *()> workflow_id_provider)
{
    if (!sidebar || !workflow_id_provider)
        return;

    auto *list = sidebar->findChild<QListWidget *>("workflowNodesList");
    if (!list)
        return;

    for (int row = 0; row < list->count(); ++row) {
        auto *item = list->item(row);
        if (!item)
            continue;
        auto *button = new QPushButton(QStringLiteral("▶"), list);
        button->setToolTip(QStringLiteral("Run workflow from this node"));
        button->setFixedWidth(28);
        button->setProperty("workflowNodeId", item->data(Qt::UserRole));
        QObject::connect(button, &QPushButton::clicked, list, [button, workflow_id_provider] {
            const QByteArray workflowId = QByteArray(workflow_id_provider());
            const QByteArray nodeId = button->property("workflowNodeId").toByteArray();
            if (!workflowId.isEmpty() && !nodeId.isEmpty())
                workflow_engine_service_run_from_node(workflowId.constData(), nodeId.constData());
        });
        list->setItemWidget(item, button);
    }
}
