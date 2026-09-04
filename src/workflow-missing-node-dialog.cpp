#include "workflow-scene.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

void EditorScene::showMissingConnections()
{
    if (missingConnections_.isEmpty())
        return;

    QDialog dialog;
    dialog.setWindowTitle("Missing Connections");
    dialog.setMinimumWidth(560);

    auto *layout = new QVBoxLayout(&dialog);
    auto *heading = new QLabel(
        "The following connections point to nodes that no longer exist.");
    heading->setWordWrap(true);
    layout->addWidget(heading);

    for (const MissingConnection &connection : missingConnections_) {
        if (!connection.from)
            continue;

        auto *row = new QHBoxLayout;
        const QString text = QString("%1  →  %2  [%3]")
                                 .arg(connection.from->nodeName())
                                 .arg(connection.targetId)
                                 .arg(connection.type);
        auto *label = new QLabel(text);
        label->setWordWrap(true);
        row->addWidget(label, 1);

        auto *remove = new QPushButton("Delete Connection");
        row->addWidget(remove);

        const QString targetId = connection.targetId;
        const QString type = connection.type;
        NodeItem *from = connection.from;
        connect(remove, &QPushButton::clicked, &dialog,
                [this, &dialog, from, targetId, type] {
                    if (deleteMissingConnection(from, targetId, type))
                        dialog.accept();
                });
        layout->addLayout(row);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dialog,
            &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}
