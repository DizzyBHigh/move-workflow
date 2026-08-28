#include "workflow-editor-sidebar-icons.h"

#include <QPainter>
#include <QPixmap>
#include <QPolygonF>

QIcon workflow_editor_sidebar_node_type_icon(workflow_node_type_t type)
{
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor background = type == WORKFLOW_NODE_TRIGGER ? QColor("#2e9d62") : QColor("#3478c7");
    painter.setBrush(background);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(0, 0, 18, 18), 3, 3);
    painter.setBrush(Qt::white);
    if (type == WORKFLOW_NODE_TRIGGER) {
        QPolygonF bolt;
        bolt << QPointF(10, 2) << QPointF(5, 10) << QPointF(9, 10)
             << QPointF(7, 16) << QPointF(14, 7) << QPointF(10, 7);
        painter.drawPolygon(bolt);
    } else {
        QPolygonF action;
        action << QPointF(4, 9) << QPointF(10, 3) << QPointF(16, 9) << QPointF(10, 15);
        painter.drawPolygon(action);
    }
    return QIcon(pixmap);
}
