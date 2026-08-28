#include "workflow-editor-view.h"

#include <QGraphicsScene>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>

WorkflowGraphicsView::WorkflowGraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setRenderHint(QPainter::Antialiasing); setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse); setDragMode(QGraphicsView::RubberBandDrag);
    setBackgroundBrush(QColor(10,16,22)); setFrameShape(QFrame::NoFrame);
    setStyleSheet("QGraphicsView{background:#0a1016;border:0;} QScrollBar:vertical,QScrollBar:horizontal{background:#0e151c;border:0;} QScrollBar::handle:vertical,QScrollBar::handle:horizontal{background:#2b3946;border-radius:4px;min-height:24px;min-width:24px;} QScrollBar::handle:hover{background:#3c5062;} QScrollBar::add-line,QScrollBar::sub-line{background:none;border:0;}");
    runtimeTimer_ = new QTimer(this);
    connect(runtimeTimer_, &QTimer::timeout, this, [this]() { if (this->scene()) this->scene()->update(); });
    runtimeTimer_->start(50);
}

void WorkflowGraphicsView::zoomIn(){scale(1.15,1.15);updateZoomLabel();}
void WorkflowGraphicsView::zoomOut(){scale(1.0/1.15,1.0/1.15);updateZoomLabel();}
void WorkflowGraphicsView::resetZoom(){resetTransform();updateZoomLabel();}
void WorkflowGraphicsView::fitAll()
{if(!scene()||scene()->items().isEmpty()){resetZoom();return;}const QRectF bounds=scene()->itemsBoundingRect().adjusted(-80,-80,80,80);if(bounds.isValid()&&!bounds.isEmpty())fitInView(bounds,Qt::KeepAspectRatio);updateZoomLabel();}
void WorkflowGraphicsView::setZoomLabel(QLabel *label){zoomLabel_=label;updateZoomLabel();}
void WorkflowGraphicsView::wheelEvent(QWheelEvent *event){const int delta=event->angleDelta().y();if(!delta){QGraphicsView::wheelEvent(event);return;}const qreal factor=delta>0?1.15:(1.0/1.15);scale(factor,factor);updateZoomLabel();event->accept();}
void WorkflowGraphicsView::mousePressEvent(QMouseEvent *event){if(event->button()==Qt::MiddleButton){panning_=true;panStart_=event->pos();setCursor(Qt::ClosedHandCursor);event->accept();return;}QGraphicsView::mousePressEvent(event);}
void WorkflowGraphicsView::mouseMoveEvent(QMouseEvent *event){if(panning_){const QPoint delta=event->pos()-panStart_;panStart_=event->pos();horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y());event->accept();return;}QGraphicsView::mouseMoveEvent(event);}
void WorkflowGraphicsView::mouseReleaseEvent(QMouseEvent *event){if(event->button()==Qt::MiddleButton&&panning_){panning_=false;unsetCursor();event->accept();return;}QGraphicsView::mouseReleaseEvent(event);}
void WorkflowGraphicsView::updateZoomLabel(){if(zoomLabel_)zoomLabel_->setText(QString("%1%").arg(qRound(transform().m11()*100.0)));}