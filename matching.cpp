//此示例应用程序显示使用与形状模型匹配的模式来定位对象。 此外，它还展示了如何使用检测到的对象位置和旋转来构建检查任务的搜索空间。 在该特定示例中，IC上的打印用于找到IC。 从找到的位置和旋转，构造两个测量矩形以测量IC的引线之间的间隔。 由于本例中使用的照明，引线在几个位置和旋转处具有255的饱和灰度值，这扩大了引线的表观宽度，因此似乎减小了引线之间的间距，尽管使用相同的板 在所有图像中。
#include "matching.h"
#include <QGridLayout>
#pragma execution_character_set("utf-8")
// 构造函数：创建GUI
Matching::Matching(QWidget *parent)
    : QWidget(parent), Timer(-1)
{
    // 按钮
    CreateButton = new QPushButton(tr("创建模型"),this);
    connect(CreateButton,SIGNAL(clicked()),SLOT(Create()));

    StartButton  = new QPushButton(tr("开始"),this);
    StartButton->setEnabled(false);
    connect(StartButton,SIGNAL(clicked()),SLOT(Start()));

    StopButton  = new QPushButton(tr("停止"),this);
    StopButton->setEnabled(false);
    connect(StopButton, SIGNAL(clicked()),SLOT(Stop()));

    ArbitrarilyDryButton = new QPushButton(tr("绘画"),this);
    connect(ArbitrarilyDryButton, SIGNAL(clicked()),SLOT(ArbitrarilyDry()));

    // 标签
    QLabel *match_time    = new QLabel(tr("匹配:"),this);
    QLabel *match_time2   = new QLabel(tr("时间:"),this);
    MatchTimeLabel        = new QLabel("        ",this);
    QLabel *match_score   = new QLabel(tr("评分:  "),this);
    MatchScoreLabel       = new QLabel("        ",this);
    QLabel *meas_time     = new QLabel(tr("测量:"),this);
    QLabel *meas_time2    = new QLabel(tr("时间:"),this);
    MeasTimeLabel         = new QLabel("        ",this);
    QLabel *num_leads     = new QLabel(tr("引线数量:  "),this);
    NumLeadsLabel         = new QLabel("        ",this);
    QLabel *min_lead_dist = new QLabel(tr("最小引导距离:  "),this);
    MinLeadsDistLabel     = new QLabel("        ",this);

    // 标签 MVTec
    QLabel *MvtecLabel    = new QLabel(("匹配和测量演示"),this);
    MvtecLabel->setFont(QFont(NULL,10,QFont::Bold ));
    // HALCON小部件的解释
    QLabel *DispHintLabel = new QLabel(
        "变焦：鼠标滚轮; 移动：鼠标左键; 重置：双击", this);



    // 布局
    // Topmost VBoxLayout
    QVBoxLayout *TopBox = new QVBoxLayout(this);

    // MVTec label layout in TopBox
    QHBoxLayout *Mvtec  = new QHBoxLayout;
    Mvtec->addStretch(1);
    Mvtec->addWidget(MvtecLabel);
    Mvtec->addStretch(1);

    // TopVBox in TopBox
    QVBoxLayout *TopVBox = new QVBoxLayout;

    // HBoxDispAndButtons in TopVBox
    QHBoxLayout *HBoxDispAndButtons = new QHBoxLayout;

    // Disp: HALCON window widget in HBoxDispAndButtons
    Disp = new QHalconWindow(this);
    Disp->setMinimumSize(50,50);

    // One layout for HALCON widget and hint label
    QVBoxLayout *DispVBox = new QVBoxLayout;
    DispVBox->addWidget(Disp, 1);
    DispVBox->addSpacing(8);
    DispVBox->addWidget(DispHintLabel);

    // Buttons in HBoxDispAndButtons
    QVBoxLayout *Buttons = new QVBoxLayout;
    Buttons->addWidget(CreateButton);
    Buttons->addSpacing(8);
    Buttons->addWidget(StartButton);
    Buttons->addSpacing(8);
    Buttons->addWidget(StopButton);
    Buttons->addSpacing(8);
    Buttons->addWidget(ArbitrarilyDryButton);
    Buttons->addStretch(1);

    // HBoxDispAndButtons
    HBoxDispAndButtons->addSpacing(15);
    HBoxDispAndButtons->addLayout(DispVBox, 1);
    HBoxDispAndButtons->addSpacing(15);
    HBoxDispAndButtons->addLayout(Buttons);
    HBoxDispAndButtons->addSpacing(15);

    // HBoxLabels in TopVBox
    QHBoxLayout *HBoxLabels = new QHBoxLayout;
    // Labels in HBoxLabels
    QGridLayout *Labels = new QGridLayout();
    Labels->addWidget(match_time,0,0);
    Labels->addWidget(match_time2,0,1);
    Labels->addWidget(MatchTimeLabel,0,2);
    Labels->addWidget(match_score,0,3);
    Labels->addWidget(MatchScoreLabel,0,4);
    Labels->addWidget(meas_time,1,0);
    Labels->addWidget(meas_time2,1,1);
    Labels->addWidget(MeasTimeLabel,1,2);
    Labels->addWidget(num_leads,1,3);
    Labels->addWidget(NumLeadsLabel,1,4);
    Labels->addWidget(min_lead_dist,1,5);
    Labels->addWidget(MinLeadsDistLabel,1,6);

    // End Labels
    HBoxLabels->addSpacing(15);
    HBoxLabels->addLayout(Labels);
    HBoxLabels->addSpacing(130);
    // End HBoxLabels
    TopVBox->addLayout(HBoxDispAndButtons);
    TopVBox->addSpacing(15);
    TopVBox->addLayout(HBoxLabels);
    // End TopVBox
    TopBox->addSpacing(15);
    TopBox->addLayout(TopVBox);
    TopBox->addSpacing(15);
    TopBox->addLayout(Mvtec);
    TopBox->addSpacing(10);
    // End TopBox
}


// 当用户通过单击窗口管理器修饰中的关闭按钮关闭应用程序时，将调用析构函数。
Matching::~Matching(void)
{
    using namespace HalconCpp;

    // 关闭所有已分配的HALCON资源。
    CloseFramegrabber(FGHandle);
    if (Timer != -1)
    {
        killTimer(Timer);
        Timer = -1;
    }
}


// 打开采集卡并抓取初始图像
void Matching::InitFg(void)
{
    using namespace HalconCpp;

    // 打开采集卡并抓取初始图像
    OpenFramegrabber("File",1,1,0,0,0,0,"default",-1,"default",-1,"default",
                     "board/board.seq","default",-1,1,&FGHandle);    
    GrabImage(&Image,FGHandle);    
    Image.GetImageSize(&ImageWidth, &ImageHeight);
    Disp->GetHalconBuffer()->SetPart(0, 0, ImageHeight-1, ImageWidth-1);

    Disp->GetHalconBuffer()->SetLineWidth(3);
    Disp->GetHalconBuffer()->DispObj(Image);
    Disp->GetHalconBuffer()->FlushBuffer();
}




// 创建形状模型
void Matching::Create(void)
{
    using namespace HalconCpp;

    HalconCpp::HTuple Area;
    HalconCpp::HTuple Row1, Column1, Row2, Column2;
    HalconCpp::HTuple RectPhi;

    HalconCpp::HObject Rectangle0, Rectangle1, Rectangle2;
    HImage   ImageReduced, ShapeModelImage;

    // 通过将创建按钮设置为不敏感来防止模型生成两次。
    CreateButton->setEnabled(false);
    CreateButton->repaint();
    setCursor(Qt::WaitCursor);
    // 使用以下四个坐标从矩形生成模型：
    Row1 = 188;
    Column1 = 182;
    Row2 = 298;
    Column2 = 412;

    GenRectangle1(&Rectangle0,Row1,Column1,Row2,Column2);
    AreaCenter(Rectangle0,&Area,&ModelRow,&ModelColumn);

    // 计算测量矩形相对于模型中心的坐标。
    Rect1Row = ModelRow-102;
    Rect1Col = ModelColumn+5;
    Rect2Row = ModelRow+107;
    Rect2Col = ModelColumn+5;
    RectPhi = 0;
    RectLength1 = 170;
    RectLength2 = 5;
    // 生成两个测量矩形以用于可视化目的。
    GenRectangle2(&Rectangle1,Rect1Row,Rect1Col,RectPhi,RectLength1,
                  RectLength2);
    GenRectangle2(&Rectangle2,Rect2Row,Rect2Col,RectPhi,RectLength1,
                  RectLength2);

    GenRectangle1(&Rectangle0,Row1,Column1,Row2,Column2);
    AreaCenter(Rectangle0,&Area,&ModelRow,&ModelColumn);

    ReduceDomain(Image,Rectangle0,&ImageReduced);
    // 创建模型的标志性表示。 该区域将通过模型的测量位置进行变换，以便稍后进行可视化。
    InspectShapeModel(ImageReduced,&ShapeModelImage,&ShapeModelRegion,1,30);
    // 创建模型。
    ShapeModel.reset(new HShapeModel(ImageReduced,4,0,2*PI,PI/180,"none","use_polarity",30,10));

    // 显示模型和测量矩形。
    Disp->GetHalconBuffer()->SetColor("green");
    Disp->GetHalconBuffer()->DispObj(ShapeModelRegion);
    Disp->GetHalconBuffer()->SetColor("blue");
    Disp->GetHalconBuffer()->SetDraw("margin");
    Disp->GetHalconBuffer()->DispObj(Rectangle1);
    Disp->GetHalconBuffer()->DispObj(Rectangle2);
    Disp->GetHalconBuffer()->SetDraw("fill");
    Disp->GetHalconBuffer()->FlushBuffer();
    // 允许用户开始匹配。
    StartButton->setEnabled(true);
    setCursor(Qt::ArrowCursor);
}


// 抓取下一张图片并进行匹配
void Matching::StartMatching(void)
{
    using namespace HalconCpp;
    double   S1, S2;
    HTuple   Rect1RowCheck, Rect1ColCheck, Rect2RowCheck, Rect2ColCheck;
    HTuple   MeasureHandle1, MeasureHandle2, NumLeads;
    HTuple   RowCheck, ColumnCheck, AngleCheck, Score, HomMat2D, MinDistance;
    HTuple   RowEdgeFirst1, ColumnEdgeFirst1, AmplitudeFirst1;
    HTuple   RowEdgeSecond1, ColumnEdgeSecond1, AmplitudeSecond1;
    HTuple   IntraDistance1, InterDistance1;
    HTuple   RowEdgeFirst2, ColumnEdgeFirst2, AmplitudeFirst2;
    HTuple   RowEdgeSecond2, ColumnEdgeSecond2, AmplitudeSecond2;
    HTuple   IntraDistance2, InterDistance2;
    HObject  ShapeModelTrans;
    HObject  Rectangle1, Rectangle2;
    HImage   Image;
    char     buf[MAX_STRING];
    QString  string;

    GrabImage(&Image,FGHandle);
    // 请注意，所有显示操作都是使用缓冲区窗口调用的。此缓冲区窗口将被复制到此函数末尾的可见窗口中。 这导致无闪烁显示结果。
    Disp->GetHalconBuffer()->DispObj(Image);
    // 在当前图像中找到IC。
    S1 = HSystem::CountSeconds();
    ShapeModel->FindShapeModel(Image, 0, 2*PI, 0.7, 1, 0.5, "least_squares", 4,
                               0.7, &RowCheck,&ColumnCheck,&AngleCheck,&Score);
    S2 = HSystem::CountSeconds();
    // 使用实际时间更新匹配时间标签。
    sprintf_s(buf,"%5.2f",(S2-S1)*1000);
    string = buf;
    MatchTimeLabel->setText(string);
    if (Score.Length() == 1)
    {
        // 使用测量的分数更新匹配的分数标签。
        sprintf_s(buf,"%7.5f",(double)Score[0]);
        string = buf;
        MatchScoreLabel->setText(string);
        // 旋转模型以进行可视化。
        VectorAngleToRigid(ModelRow,ModelColumn,0,RowCheck,ColumnCheck,AngleCheck,
                           &HomMat2D);
        AffineTransRegion(ShapeModelRegion,&ShapeModelTrans,HomMat2D,"false");
        Disp->GetHalconBuffer()->SetColor("green");
        Disp->GetHalconBuffer()->DispObj(ShapeModelTrans);
        // 计算测量矩形的参数。
        AffineTransPixel(HomMat2D,Rect1Row,Rect1Col,&Rect1RowCheck,
                         &Rect1ColCheck);
        AffineTransPixel(HomMat2D,Rect2Row,Rect2Col,&Rect2RowCheck,
                         &Rect2ColCheck);
        // 出于可视化目的，将两个矩形生成为区域并显示它们。
        GenRectangle2(&Rectangle1,Rect1RowCheck,Rect1ColCheck,AngleCheck,
                      RectLength1,RectLength2);
        GenRectangle2(&Rectangle2,Rect2RowCheck,Rect2ColCheck,AngleCheck,
                      RectLength1,RectLength2);
        Disp->GetHalconBuffer()->SetColor("blue");
        Disp->GetHalconBuffer()->SetDraw("margin");
        Disp->GetHalconBuffer()->DispObj(Rectangle1);
        Disp->GetHalconBuffer()->DispObj(Rectangle2);
        Disp->GetHalconBuffer()->SetDraw("fill");
        // 做实际测量。
        S1 = HSystem::CountSeconds();
        GenMeasureRectangle2(Rect1RowCheck,Rect1ColCheck,AngleCheck,
                             RectLength1,RectLength2,ImageWidth,ImageHeight,"bilinear",
                             &MeasureHandle1);
        GenMeasureRectangle2(Rect2RowCheck,Rect2ColCheck,AngleCheck,
                             RectLength1,RectLength2,ImageWidth,ImageHeight,"bilinear",
                             &MeasureHandle2);
        MeasurePairs(Image,MeasureHandle1,2,90,"positive","all",
                     &RowEdgeFirst1,&ColumnEdgeFirst1,&AmplitudeFirst1,
                     &RowEdgeSecond1,&ColumnEdgeSecond1,&AmplitudeSecond1,
                     &IntraDistance1,&InterDistance1);
        MeasurePairs(Image,MeasureHandle2,2,90,"positive","all",
                     &RowEdgeFirst2,&ColumnEdgeFirst2,&AmplitudeFirst2,
                     &RowEdgeSecond2,&ColumnEdgeSecond2,&AmplitudeSecond2,
                     &IntraDistance2,&InterDistance2);
        CloseMeasure(MeasureHandle1);
        CloseMeasure(MeasureHandle2);

        S2 = HSystem::CountSeconds();
        // 显示测量结果。
        Disp->GetHalconBuffer()->SetColor("red");
        Disp->GetHalconBuffer()->DispLine(
            RowEdgeFirst1-RectLength2*AngleCheck.TupleCos(),
            ColumnEdgeFirst1-RectLength2*AngleCheck.TupleSin(),
            RowEdgeFirst1+RectLength2*AngleCheck.TupleCos(),
            ColumnEdgeFirst1+RectLength2*AngleCheck.TupleSin());
        Disp->GetHalconBuffer()->DispLine(
            RowEdgeSecond1-RectLength2*AngleCheck.TupleCos(),
            ColumnEdgeSecond1-RectLength2*AngleCheck.TupleSin(),
            RowEdgeSecond1+RectLength2*AngleCheck.TupleCos(),
            ColumnEdgeSecond1+RectLength2*AngleCheck.TupleSin());
        Disp->GetHalconBuffer()->DispLine(
            RowEdgeFirst2-RectLength2*AngleCheck.TupleCos(),
            ColumnEdgeFirst2-RectLength2*AngleCheck.TupleSin(),
            RowEdgeFirst2+RectLength2*AngleCheck.TupleCos(),
            ColumnEdgeFirst2+RectLength2*AngleCheck.TupleSin());
        Disp->GetHalconBuffer()->DispLine(
            RowEdgeSecond2-RectLength2*AngleCheck.TupleCos(),
            ColumnEdgeSecond2-RectLength2*AngleCheck.TupleSin(),
            RowEdgeSecond2+RectLength2*AngleCheck.TupleCos(),
            ColumnEdgeSecond2+RectLength2*AngleCheck.TupleSin());

        Disp->GetHalconBuffer()->SetDraw("fill");
        // 使用实际时间更新测量时间标签。
        sprintf_s(buf,"%5.2f",(S2-S1)*1000);
        string = buf;
        MeasTimeLabel->setText(string);
        // 使用测量的引线数更新引线编号标签。
        NumLeads = IntraDistance1.Length()+IntraDistance2.Length();

        sprintf_s(buf,"%2ld",(long)((Hlong)NumLeads[0]));
        string = buf;

        NumLeadsLabel->setText(string);
        // 使用测量的最小距离更新引导距离标签。
        MinDistance = (InterDistance1.TupleConcat(InterDistance2)).TupleMin();
        sprintf_s(buf,"%6.3f",(double)MinDistance[0]);
        string = buf;
        MinLeadsDistLabel->setText(string);

    }
    Disp->GetHalconBuffer()->FlushBuffer();
}


// 在:: Start（）中启动Timer后，将连续调用此函数
void Matching::timerEvent(QTimerEvent*)
{
    StartMatching();
}


// 开始连续匹配
void Matching::Start(void)
{
    StartButton->setEnabled(false);
    StopButton->setEnabled(true);
    // 启动计时器 - > :: timerEvent（）被连续调用
    Timer = startTimer(20);
}


// 停止连续匹配
void Matching::Stop(void)
{
    StartButton->setEnabled(true);
    StopButton->setEnabled(false);
    // 杀死计时器
    if (Timer != -1)
    {
        killTimer(Timer);
        Timer = -1;
    }
}

#include <qdebug.h>
//roi绘画选取
void Matching::ArbitrarilyDry(void)
{
    using namespace HalconCpp;

    Disp->setCanMove(false);

    HalconCpp::HObject  temp;
    HalconCpp::HTuple  hv_WindowID;//窗口句柄
    Hlong MainWndID;//当前窗口句柄
    Disp->GetHalconBuffer()->DispObj(Image);
    MainWndID = Disp->winId();
    OpenWindow(0,0,Disp->width(),Disp->height(),
               MainWndID,"","",&hv_WindowID);
    HDevWindowStack::Push(hv_WindowID);

    double row1, col1, row2, col2;
    Disp->GetPartFloat(&row1, &col1, &row2, &col2);
    SetPart(hv_WindowID,HalconCpp::HTuple(row1), HalconCpp::HTuple(col1),
            HalconCpp::HTuple(row2), HalconCpp::HTuple(col2));
    DispObj(Image, HDevWindowStack::GetActive());
    SetColor(HDevWindowStack::GetActive(),"red");
    DrawRegion(&temp,hv_WindowID);
    HDevWindowStack::Pop();





    Disp->GetHalconBuffer()->DispObj(Image);
    Disp->GetHalconBuffer()->SetColor("blue");
    Disp->GetHalconBuffer()->SetDraw("margin");
    Disp->GetHalconBuffer()->SetLineWidth(1);
    Disp->GetHalconBuffer()->DispRegion(temp);
    Disp->GetHalconBuffer()->FlushBuffer();


    Disp->setCanMove(true);


}
