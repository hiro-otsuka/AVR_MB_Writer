#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>
#include <QSettings>

int main(int argc, char *argv[])
{
  QSettings* aSetting = new QSettings("AVR_MB.ini", QSettings::IniFormat);
  QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());
  QApplication a(argc, argv);
  MainWindow w(aSetting);
  w.show();

  int ret = a.exec();
  delete aSetting;
  return ret;
}
