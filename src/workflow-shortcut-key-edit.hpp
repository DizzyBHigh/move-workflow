#pragma once

#include <obs.h>

#include <QWidget>

class WorkflowShortcutKeyEdit final : public QWidget {
    Q_OBJECT

public:
    explicit WorkflowShortcutKeyEdit(QWidget *parent = nullptr);

    obs_key_combination_t combination() const;
    void setCombination(obs_key_combination_t combination);

signals:
    void combinationChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateDisplay();

    obs_key_combination_t combination_{0, OBS_KEY_NONE};
    bool capturing_ = false;
};
