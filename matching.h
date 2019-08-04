
#pragma once

#include <qlabel.h>
#include <qpushbutton.h>


#include <QScopedPointer>
#include "qhalconwindow.h"

class Matching: public QWidget
{
    Q_OBJECT

    //构建、析构、初始化
public:
    Matching(QWidget *parent=0);
    virtual ~Matching(void);
    void InitFg(void);

    //按钮事件
protected slots:
    void Create(void);
    void Start(void);
    void Stop(void);
    void ArbitrarilyDry(void);

    //计时
protected:
    void timerEvent(QTimerEvent*);
    void StartMatching(void);

private:
    // HALCON 变量
    Hlong ImageWidth, ImageHeight;
    HalconCpp::HTuple FGHandle;
    HalconCpp::HImage Image;
    QScopedPointer<HalconCpp::HShapeModel> ShapeModel;
    HalconCpp::HTuple ModelRow, ModelColumn;
    HalconCpp::HTuple Rect1Row, Rect1Col, Rect2Row, Rect2Col;
    HalconCpp::HTuple RectLength1, RectLength2;
    HalconCpp::HObject ShapeModelRegion;
    // GUI 成员
    QLabel *MatchTimeLabel, *MatchScoreLabel, *MeasTimeLabel;
    QLabel *NumLeadsLabel, *MinLeadsDistLabel;
    QPushButton *CreateButton, *StartButton, *StopButton, *ArbitrarilyDryButton;
    QHalconWindow *Disp;

    // Timer
    long Timer;
};
