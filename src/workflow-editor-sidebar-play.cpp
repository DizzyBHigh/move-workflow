#include "workflow-editor-sidebar-play.hpp"
#include "workflow-engine-service.h"

#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QWidget>

namespace {
class PlayDelegate final : public QStyledItemDelegate {
public:
    PlayDelegate(QListWidget *list, std::function<const char *()> provider)
        : QStyledItemDelegate(list), list_(list), provider_(std::move(provider)) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem itemOption(option);
        itemOption.rect.setRight(option.rect.right() - 34);
        QStyledItemDelegate::paint(painter, itemOption, index);
        const QRect buttonRect(option.rect.right() - 32, option.rect.top() + 2, 28,
                               option.rect.height() - 4);
        QStyleOptionButton button;
        button.rect = buttonRect;
        button.state = QStyle::State_Enabled;
        if (option.state & QStyle::State_MouseOver)
            button.state |= QStyle::State_MouseOver;
        if (option.state & QStyle::State_Selected)
            button.state |= QStyle::State_HasFocus;
        option.widget->style()->drawControl(QStyle::CE_PushButton, &button, painter,
                                            option.widget);
        painter->drawText(buttonRect, Qt::AlignCenter, QStringLiteral("▶"));
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override
    {
        if (event->type() != QEvent::MouseButtonRelease)
            return QStyledItemDelegate::editorEvent(event, nullptr, option, index);
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton ||
            !playRect(option.rect).contains(mouse->position().toPoint()))
            return QStyledItemDelegate::editorEvent(event, nullptr, option, index);
        const QByteArray workflowId = QByteArray(provider_ ? provider_() : nullptr);
        const QByteArray nodeId = index.data(Qt::UserRole).toByteArray();
        if (!workflowId.isEmpty() && !nodeId.isEmpty())
            workflow_engine_service_run_from_node(workflowId.constData(), nodeId.constData());
        return true;
    }

private:
    static QRect playRect(const QRect &rect)
    {
        return QRect(rect.right() - 32, rect.top() + 2, 28, rect.height() - 4);
    }
    QListWidget *list_ = nullptr;
    std::function<const char *()> provider_;
};
}

void workflow_editor_sidebar_install_play_buttons(
    QWidget *sidebar, std::function<const char *()> workflow_id_provider)
{
    if (!sidebar || !workflow_id_provider)
        return;
    auto *list = sidebar->findChild<QListWidget *>();
    if (!list)
        return;
    list->setItemDelegate(new PlayDelegate(list, std::move(workflow_id_provider)));
}
