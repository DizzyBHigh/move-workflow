#include "workflow-connection-editor.h"

#include <QAction>
#include <QMenu>

bool WorkflowConnectionEditor::showMenu(QGraphicsPathItem *line, const QPointF &globalPos,
                                        const QString &currentType, Handler handler, void *context)
{
    if (!line || !handler)
        return false;

    QMenu menu;
    QAction *simultaneous = menu.addAction("Change to Simultaneous");
    QAction *next = menu.addAction("Change to Next");
    QAction *shortcut = menu.addAction("Change to Shortcut");
    menu.addSeparator();
    QAction *remove = menu.addAction("Delete Connection");

    simultaneous->setEnabled(currentType != "Simultaneous");
    next->setEnabled(currentType != "Next Action");
    shortcut->setEnabled(currentType != "Shortcut");

    QAction *selected = menu.exec(globalPos);
    if (selected == remove) {
        handler(context, "__delete__");
        return true;
    }
    if (selected == simultaneous) {
        handler(context, "Simultaneous");
        return true;
    }
    if (selected == next) {
        handler(context, "Next");
        return true;
    }
    if (selected == shortcut) {
        handler(context, "Shortcut");
        return true;
    }
    return false;
}
