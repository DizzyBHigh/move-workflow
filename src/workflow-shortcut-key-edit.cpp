#include "workflow-shortcut-key-edit.hpp"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

namespace {
uint32_t qtModifiers(Qt::KeyboardModifiers modifiers)
{
    uint32_t result = 0;
    if (modifiers & Qt::ShiftModifier)
        result |= INTERACT_SHIFT_KEY;
    if (modifiers & Qt::ControlModifier)
        result |= INTERACT_CONTROL_KEY;
    if (modifiers & Qt::AltModifier)
        result |= INTERACT_ALT_KEY;
    if (modifiers & Qt::MetaModifier)
        result |= INTERACT_COMMAND_KEY;
    return result;
}

bool isModifierKey(obs_key_t key)
{
    switch (key) {
    case OBS_KEY_SHIFT:
    case OBS_KEY_CONTROL:
    case OBS_KEY_ALT:
    case OBS_KEY_META:
        return true;
    default:
        return false;
    }
}
}

WorkflowShortcutKeyEdit::WorkflowShortcutKeyEdit(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(28);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    updateDisplay();
}

obs_key_combination_t WorkflowShortcutKeyEdit::combination() const
{
    return combination_;
}

void WorkflowShortcutKeyEdit::setCombination(obs_key_combination_t combination)
{
    combination_ = combination;
    updateDisplay();
}

void WorkflowShortcutKeyEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        capturing_ = false;
        updateDisplay();
        event->accept();
        return;
    }

    const obs_key_t key = obs_key_from_virtual_key(event->nativeVirtualKey());
    if (key == OBS_KEY_NONE || isModifierKey(key)) {
        event->accept();
        return;
    }

    combination_.modifiers = qtModifiers(event->modifiers());
    combination_.key = key;
    capturing_ = false;
    updateDisplay();
    emit combinationChanged();
    event->accept();
}

void WorkflowShortcutKeyEdit::keyReleaseEvent(QKeyEvent *event)
{
    event->accept();
}

void WorkflowShortcutKeyEdit::focusOutEvent(QFocusEvent *event)
{
    capturing_ = false;
    updateDisplay();
    QWidget::focusOutEvent(event);
}

void WorkflowShortcutKeyEdit::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        capturing_ = true;
        setFocus(Qt::MouseFocusReason);
        updateDisplay();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void WorkflowShortcutKeyEdit::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 3, 3);

    const QString text = capturing_ ? tr("Press shortcut…") :
                                     (combination_.key == OBS_KEY_NONE ?
                                          tr("Click and press shortcut…") :
                                          QString::fromUtf8(obs_key_combination_to_str(combination_)));
    painter.drawText(rect().adjusted(8, 0, -8, 0), Qt::AlignVCenter, text);
}

void WorkflowShortcutKeyEdit::updateDisplay()
{
    update();
}
