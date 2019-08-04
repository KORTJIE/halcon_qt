#include <QMouseEvent>
#include <QWheelEvent>

#include "qhalconwindow.h"

#include <cmath>

Herror __stdcall ContentUpdateCallback(void *context)
{
    //__stdcall：函数参数由右向左入栈,函数调用结束后由被调用函数清除栈内数据。
    // 如果启用了自动刷新，则此回调将调用flush_buffer（默认）在每次更改图形缓冲区后调用它
    QHalconWindow* hwindow = (QHalconWindow*)context;

    // 在Qt线程中安排重绘
    hwindow->update();

    return H_MSG_OK;
}

QHalconWindow::QHalconWindow(QWidget *parent, long Width, long Height)
    : QWidget(parent), lastMousePos(-1, -1),CanMove(true)
{
    show();
    resize(Width,Height);

    // 初始化HALCON缓冲区窗口
    halconBuffer.reset(new HalconCpp::HWindow(0, 0, 100, 100, 0, "buffer", ""));

    //SetWindowParam允许设置打开窗口的不同参数
    // 打开图形堆栈，因此图像和区域在缩放或调整大小后仍然保持不变
    halconBuffer->SetWindowParam("graphics_stack", "true");
    // 打开明确的冲洗
    halconBuffer->SetWindowParam("flush", "true");
    // 注册更新回调
    halconBuffer->SetContentUpdateCallback((void*)&ContentUpdateCallback, this);
}

// 每当调整QHalconWindow小部件的大小时，调整HALCON窗口的大小
void QHalconWindow::resizeEvent(QResizeEvent* event)
{
    if(!CanMove)
        return;
    Q_UNUSED(event);
    // 将HALCON窗口设置为新大小。
    halconBuffer->SetWindowExtents(0,0,width(),height());
    // 启动重绘缓冲区。
    // (这使用graphics_stack来获取最后的图像和对象)
    halconBuffer->FlushBuffer();
}

void QHalconWindow::paintEvent(QPaintEvent *event)
{
    if(!CanMove)
        return;
    using namespace HalconCpp;
    Q_UNUSED(event);

    HString type;
    Hlong   width, height;
    //获取缓冲区的内容
    HImage image = halconBuffer->DumpWindowImage();
    // 将缓冲区转换为Qt中使用的格式
    HImage imageInterleaved = image.InterleaveChannels("argb", "match", 0);
    // 获取原始图像数据的访问权限
    unsigned char* pointer = (unsigned char*)imageInterleaved.GetImagePointer1(&type, &width, &height);
    // 从数据创建QImage
    QImage qimage(pointer, width/4, height, QImage::Format_RGB32);

    // 将图像绘制到小部件
    QPainter painter(this);
    painter.drawImage(QPoint(0, 0), qimage);

}

void QHalconWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(!CanMove)
        return;
    if ((event->buttons() == Qt::LeftButton) && lastMousePos.x() != -1)
    {
        QPoint delta = lastMousePos - event->globalPos();

        // 缩放增量到图像缩放系数
        double scalex = (lastCol2 - lastCol1 + 1) / (double)width();
        double scaley = (lastRow2 - lastRow1 + 1) / (double)height();
        try
        {
            // 设置新的可见部分
            SetPartFloat(lastRow1 + (delta.y() * scaley),
                         lastCol1 + (delta.x() * scalex),
                         lastRow2 + (delta.y() * scaley),
                         lastCol2 + (delta.x() * scalex));
            // 启动重绘（）
            halconBuffer->FlushBuffer();
        }
        catch (HalconCpp::HOperatorException)
        {
            // 如果零件图像移动到窗口之外，则可能发生这种情况
        }
    }
}

void QHalconWindow::mousePressEvent(QMouseEvent *event)
{
    if(!CanMove)
        return;
    // 保存最后一个鼠标位置和图像部分
    GetPartFloat(&lastRow1, &lastCol1, &lastRow2, &lastCol2);
    lastMousePos = event->globalPos();
}

void QHalconWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(!CanMove)
        return;
    Q_UNUSED(event);
    // 未设置参考鼠标位置
    lastMousePos = QPoint(-1, -1);
}

void QHalconWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(!CanMove)
        return;
    Q_UNUSED(event);//没有实质性的作用，用来避免编译器警告
    if (event->buttons() == Qt::LeftButton)
    {
        // 重置图像部分
        halconBuffer->SetPart(0, 0, -1, -1);
        halconBuffer->FlushBuffer();
    }
}

void QHalconWindow::wheelEvent(QWheelEvent *event)
{
    if(!CanMove)
        return;


    // event-> delta（）是120的倍数。对于较大的倍数，用户将轮子旋转多个槽口。
    int num_notch = std::abs(event->delta()) / 120;
    double factor = (event->delta() > 0) ? std::sqrt(2.0) : 1.0 / std::sqrt(2.0);
    while (num_notch > 1)
    {
        factor = factor * ((event->delta() > 0) ? std::sqrt(2.0) : 1.0 / std::sqrt(2.0));
        num_notch--;
    }

    // 得到缩放中心
    double centerRow, centerCol;
    halconBuffer->ConvertCoordinatesWindowToImage(event->y(), event->x(), &centerRow, &centerCol);
    // 获取当前图像部分
    double row1, col1, row2, col2;
    GetPartFloat(&row1, &col1, &row2, &col2);
    // 在中心周围
    double left = centerRow - row1;
    double right = row2 - centerRow;
    double top = centerCol - col1;
    double buttom = col2 - centerCol;
    double newRow1 = centerRow - left * factor;
    double newRow2 = centerRow + right * factor;
    double newCol1 = centerCol - top * factor;
    double newCol2 = centerCol + buttom * factor;
    try
    {
        SetPartFloat(newRow1, newCol1, newRow2, newCol2);
        halconBuffer->FlushBuffer();
    }
    catch (HalconCpp::HOperatorException)
    {
        // 如果零件太小或太大，可能会发生这种情况
    }
}





void QHalconWindow::GetPartFloat(double *row1, double *col1, double *row2, double *col2)
{
    // 要从get_part获取浮点值，请使用HTuple参数
    HalconCpp::HTuple trow1, tcol1, trow2, tcol2;
    halconBuffer->GetPart(&trow1, &tcol1, &trow2, &tcol2);
    *row1 = trow1.D();
    *col1 = tcol1.D();
    *row2 = trow2.D();
    *col2 = tcol2.D();
}

void QHalconWindow::setCanMove(bool a)
{
    CanMove=a;
}

void QHalconWindow::SetPartFloat(double row1, double col1, double row2, double col2)
{
    // 将double值转换为HTuple。 否则，使用SetPart的int变体，即使在放大时也能实现平滑移动和缩放
    halconBuffer->SetPart(HalconCpp::HTuple(row1), HalconCpp::HTuple(col1),
                          HalconCpp::HTuple(row2), HalconCpp::HTuple(col2));
}
