#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QFile>
#include <QProcess>
#include <QMessageBox>
#include <QSettings>
#include <QTemporaryFile>
#include "settings.h"

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QSettings* setting, QWidget *parent = 0);
  ~MainWindow();

private slots:
  void on_btnRefresh_clicked();
  void on_btnConnect_clicked();
  void on_btnFiles_clicked();
  void on_btnAddr_clicked();
  void on_btnBinary_clicked();
  void on_btnWrite_clicked();
  void on_btnFuse_clicked();
  void on_btnElf_clicked();
  void on_btnElfWrite_clicked();
  void on_btnDelete_clicked();
  void on_btnMML2BIN_clicked();
  void on_btnMMLEdit_clicked();
  void on_btnMML_clicked();

  void on_txtAddr_editingFinished();
  void on_lstFiles_currentRowChanged(int currentRow);

  void actExit();
  void actSettings();

  void on_ProcessOut();
  void on_ProcessFinished(int code, QProcess::ExitStatus status);

  void readData();

  void on_btnEEPROM_clicked();

  void on_btnEEPROMWrite_clicked();

private:
  Ui::MainWindow *ui;
  Settings* setdialog;
  QSerialPort *serial;
  enum execMode {
    EM_END, EM_VER, EM_FIND, EM_FILES, EM_ADDR, EM_WRITE, EM_FUSE, EM_EEPROM, EM_PROG, EM_ABORT, EM_DEL, EM_RES_ADDR, EM_INS_READ, EM_INS_ADDR, EM_INS_WRITE, EM_DEL_ADDR, EM_DEL_READ, EM_DEL_ADDR2
  };
  enum lineMode {
    LM_NONE, LM_CMD, LM_RES_1, LM_RES_L, LM_MESG, LM_LIST, LM_ERR
  };
  QString IntToHex(int data);
  void ErrorMessage(QString msg);
  int  SelectYorC(QString msg);

  void StartCommand(execMode mode);
  void Proc_Kill(QProcess* target);
  void ConsoleOut(const QByteArray& buff);
  void ConnectionSet(bool isConnected);
  void EEPROMExist(bool isExist);
  void CommandRunning(bool isRunning);

  execMode nowExec;
  lineMode nowLine;

  QString  nowVer;
  QString  nowAddr;
  QString  nextAddr;
  int      nowFuse;
  int      nowSize;
  int      nowBank;
  bool     nowConnect;
  bool     nowEEPROM;
  QFile    nowFile;
  QTemporaryFile  nowTmp;

  QString  res_Line;
  QString  res_Error;
  QVector<QString>  res_List;

  QProcess* nowProc;
  QProcess* nowEditor;
  QSettings* nowSettings;
};

#endif // MAINWINDOW_H
