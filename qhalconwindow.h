
#pragma once
#include <QWidget>
#include <QPainter>
#include <QScopedPointer>
#  include "HalconCpp.h"


class QHalconWindow: public QWidget
{
    Q_OBJECT

public:
    QHalconWindow(QWidget *parent=0, long Width=0, long Height=0);
    HalconCpp::HWindow* GetHalconBuffer(void) {return halconBuffer.data();}
    void GetPartFloat(double *row1, double *col1, double *row2, double *col2);
    void setCanMove(bool a);
protected:
    void resizeEvent(QResizeEvent*);
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
private:
    void SetPartFloat(double row1, double col1, double row2, double col2);
    QScopedPointer<HalconCpp::HWindow> halconBuffer;
    QPoint lastMousePos;
    double lastRow1, lastCol1, lastRow2, lastCol2;
    bool CanMove;
};
