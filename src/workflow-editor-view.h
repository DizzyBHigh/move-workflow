#pragma once

#include <QGraphicsView>

class QLabel;

class WorkflowGraphicsView final : public QGraphicsView {
    Q_OBJECT

public:
    explicit WorkflowGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitAll();
    void setZoomLabel(QLabel *label);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updateZoomLabel();

    QLabel *zoomLabel_ = nullptr;
    bool panning_ = false;
    QPoint panStart_;
};
