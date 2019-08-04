#include "matching.h"
#include <QApplication>
#pragma execution_character_set("utf-8")


int main(int argc, char **argv)
{

  QApplication application(argc,argv);
  Matching w;

  try
  {
    w.resize(QSize(700, 500));
    w.setWindowTitle("匹配和测量演示");
    w.InitFg();
    w.show();
  }
  catch (HalconCpp::HException &exception)
  {
    fprintf(stderr, "Error #%u: %s\n",
            exception.ErrorCode(), (const char*)exception.ErrorMessage());
    exit(-1);
  }

  return application.exec();
}
