#include "workflow-node-dialog.h"

#include "workflow-model.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace {

/*
 * Dialog implementation is intentionally kept in its own translation unit.
 * The full NodeSettingsDialog implementation will be moved here in the next
 * extraction step; this file establishes the small compilation boundary first.
 */
class NodeSettingsDialog final : public QDialog {
public:
    NodeSettingsDialog(NodeItem *node, const QList<NodeItem *> &nodes, QWidget *parent)
        : QDialog(parent), node_(node), nodes_(nodes)
    {
        setWindowTitle(node && node->workflowNode()->type == WORKFLOW_NODE_TRIGGER
                           ? QStringLiteral("Edit Trigger Node")
                           : QStringLiteral("Edit Action Node"));
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(QStringLiteral("Node settings"), this));
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                             Qt::Horizontal, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }

private:
    NodeItem *node_ = nullptr;
    QList<NodeItem *> nodes_;
};

} // namespace

bool edit_node_settings(NodeItem *node,
                        const QList<NodeItem *> &nodes,
                        QWidget *parent)
{
    NodeSettingsDialog dialog(node, nodes, parent);
    return dialog.exec() == QDialog::Accepted;
}
