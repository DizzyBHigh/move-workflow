#include "workflow-editor-sidebar-play.hpp"
#include "workflow-engine-service.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

void workflow_editor_sidebar_install_play_buttons(QWidget *sidebar,
                                                   std::function<const char *()> workflow_id_provider)
{
    if (!sidebar || !workflow_id_provider)
        return;
    auto *list = dynamic_cast<QListWidget *>(sidebar);
    if (!list)
        list = sidebar->findChild<QListWidget *>("workflowNodesList");
    if (!list)
        return;
    for (int row = 0; row < list->count(); ++row) {
        auto *item = list->item(row);
        if (!item)
            continue;
        auto *container = new QWidget(list);
        auto *layout = new QHBoxLayout(container);
        layout->setContentsMargins(8, 2, 4, 2);
        layout->setSpacing(6);
        auto *icon = new QLabel(container);
        icon->setPixmap(item->icon().pixmap(16, 16));
        auto *label = new QLabel(item->text(), container);
        auto *button = new QPushButton(QStringLiteral("▶"), container);
        button->setToolTip(QStringLiteral("Run workflow from this node"));
        button->setFixedSize(28, 24);
        layout->addWidget(icon); layout->addWidget(label, 1); layout->addWidget(button);
        const QByteArray nodeId = item->data(Qt::UserRole).toByteArray();
        QObject::connect(button, &QPushButton::clicked, list, [nodeId, workflow_id_provider] {
            const QByteArray workflowId = QByteArray(workflow_id_provider());
            if (!workflowId.isEmpty() && !nodeId.isEmpty())
                workflow_engine_service_run_from_node(workflowId.constData(), nodeId.constData());
        });
        list->setItemWidget(item, container);
    }
}
