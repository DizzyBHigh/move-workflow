#pragma once

#include <QGraphicsPathItem>
#include <QPointF>
#include <QString>

class WorkflowConnectionEditor final {
public:
    using Handler = void (*)(void *context, const QString &type);

    static bool showMenu(QGraphicsPathItem *line, const QPointF &globalPos,
                         const QString &currentType, Handler handler, void *context);
};
