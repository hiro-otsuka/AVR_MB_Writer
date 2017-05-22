/*
 * mainwindow.h
 *
 * 概要：
 *  mainwindow.cpp のための定義
 *
 * 使用方法等：
 *  mainwindow.cpp を使用するモジュールから include
 *
 * ライセンス：
 *  Copyright (c) 2017, Hiro OTSUKA All rights reserved.
 *  （同梱の license.md参照 / See license.md）
 *
 * 更新履歴：
 *  2017/05/21 正式版公開(Hiro OTSUKA)
 *
 */

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
#include <QElapsedTimer>
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
  //GUIイベント用の定義
  void actExit();
  void actSettings();

  void on_btnRefresh_clicked();
  void on_btnMML_clicked();
  void on_btnBinary_clicked();
  void on_btnEEPROM_clicked();
  void on_btnElf_clicked();
  void on_btnMML2BIN_clicked();
  void on_btnMMLEdit_clicked();

  void on_btnConnect_clicked();
  void on_btnFiles_clicked();
  void on_btnAddr_clicked();

  void on_btnWrite_clicked();
  void on_btnDelete_clicked();
  void on_btnEEPROMWrite_clicked();
  void on_btnElfWrite_clicked();
  void on_btnFuse_clicked();

  void on_txtAddr_editingFinished();
  void on_lstFiles_currentRowChanged(int currentRow);

  //シグナルイベントの定義
  void on_ProcessOut();
  void on_ProcessFinished(int code, QProcess::ExitStatus status);
  void readData();

private:
  //内部オブジェクト
  Ui::MainWindow *ui;
  QSettings* nowSettings;	//iniファイル管理用オブジェクト
  Settings* setdialog;		//設定ダイアログ

  QSerialPort *serial;		//シリアルポート管理
  QProcess* nowProc;		//ツールのサブプロセス
  QProcess* nowEditor;		//テキストエディタ
  QElapsedTimer timer;		//タイマー（残り時間表示用）
  QTemporaryFile  nowTmp;	//一時ファイル（EEPROM読み込み用）

  //各種ステータス管理
  enum execMode { //実行モード
    EM_END, EM_VER, EM_FIND, EM_FILES, EM_ADDR, EM_WRITE, EM_FUSE, EM_EEPROM, EM_PROG, EM_ABORT, EM_DEL, EM_RES_ADDR, EM_INS_READ, EM_INS_ADDR, EM_TMP_WRITE, EM_INS_WRITE, EM_DELR_ADDR, EM_DEL_READ, EM_DELW_ADDR
  };
  enum lineMode { //行の状態
    LM_NONE, LM_CMD, LM_RES_1, LM_RES_L, LM_MESG, LM_LIST, LM_ERR
  };
  //文字列表示用
  QString IntToHex(int data);
  QString IntToPercent(int valNow, int valMax);

  //ダイアログ
  void ErrorMessage(QString msg);
  int  SelectYorC(QString msg);

  //コマンド処理補助用
  void StartCommand(execMode mode);
  void Proc_Kill(QProcess* target);

  //GUI補助用
  void ConsoleOut(const QByteArray& buff);
  void ConsoleReLine();
  void ConnectionSet(bool isConnected);
  void EEPROMExist(bool isExist);
  void CommandRunning(bool isRunning);

  //内部変数
  execMode nowExec;     //現在の実行モード
  lineMode nowLine;     //現在の行の状態

  QString  nowVer;      //ファームウェアのバージョン
  QString  nowAddr;     //現在処理対象としているEEPROMアドレス
  QString  nextAddr;    //次に処理対象とするEEPROMアドレス
  int      nowFuse;     //現在のFuse設定選択項目
  int      nowSize;     //現在の読み書き対象サイズ
  int      nowBank;     //現在のEEPROMバンク数
  int      nowConsole;  //現在のコンソール出力レベル
  int      nowProg;     //書き込み進捗率表示用の現在値（残サイズ）
  int      maxProg;     //書き込み進捗率表示用の最大値
  int      nowProgR;    //読み込み進捗率表示用の現在値（残サイズ）
  int      maxProgR;    //読み込み進捗率表示用の最大値
  bool     nowConnect;  //ライタ接続が必要なコマンドの有効化状態
  bool     nowEEPROM;   //EEPROM認識が必要なコマンドの有効化状態
  QFile    nowFile;     //現在処理対象としているファイル

  //結果保持用変数
  QString  res_Line;            //ライタからの1行応答を保存
  QString  res_Error;           //ライタからのエラー応答を保存
  QVector<QString>  res_List;   //ライタからのリスト応答を保存
};

#endif // MAINWINDOW_H
