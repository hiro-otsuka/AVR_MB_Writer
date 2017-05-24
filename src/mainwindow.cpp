/*
 * mainwindow.cpp
 *
 * 概要：
 *  AVR_MB_Writer のメインウィンドウ
 *
 * 使用方法等：
 *  AVR_MB_Writer を起動するプログラムにリンクして使用する
 *
 * ライセンス：
 *  Copyright (c) 2017, Hiro OTSUKA All rights reserved.
 *  （同梱の license.md参照 / See license.md）
 *
 * 更新履歴：
 *  2017/05/21 正式版公開(Hiro OTSUKA)
 *  2017/05/23 構成変更(Hiro OTSUKA) 実行モードの遷移方法などコードを再整理
 *  2017/05/24 構成変更(Hiro OTSUKA) 各モードの動作を共通化
 *
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QScrollBar>
#include <QFileDialog>
#include <QTextCodec>

// コンストラクタ・デストラクタ =====================================================================================================
//-----------------------------
//コンストラクタ
//  引数：設定ファイルオブジェクトへの参照、親オブジェクトへの参照
MainWindow::MainWindow(QSettings* setting, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  //内部オブジェクトを初期化
  ui->setupUi(this);
  nowSettings = setting;
  setdialog = new Settings(setting, this);

  serial = new QSerialPort(this);
  connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
  nowProc = new QProcess(this);
  nowEditor = new QProcess(this);
  connect(nowProc, SIGNAL(readyReadStandardOutput()), this, SLOT(on_ProcessOut()));
  connect(nowProc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(on_ProcessFinished(int, QProcess::ExitStatus)));

  nowTmp.setAutoRemove(false);

  //GUIメニューを初期化
  connect(ui->actExit, &QAction::triggered, this, &MainWindow::actExit);
  connect(ui->actSettings, &QAction::triggered, this, &MainWindow::actSettings);

  //変数（各種モード）を初期化
  nowExec = EM_END;
  nowLine = LM_NONE;

  //状態遷移用のハッシュテーブルを初期化
  // 異常系（即時終了）
  nextExec.insert(EM_END, EM_END);
  nextExec.insert(EM_ABORT, EM_END);

  // 単発コマンド群（即時終了）
  nextExec.insert(EM_VER, EM_END);
  nextExec.insert(EM_EEPROM, EM_END);
  nextExec.insert(EM_PROG, EM_END);
  nextExec.insert(EM_FUSE, EM_END);
  nextExec.insert(EM_ADDR, EM_END);             //連鎖コマンドからも開始される

  // 連鎖コマンド群（結果更新）
  nextExec.insert(EM_FILES, EM_ADDR);           //ファイル表示後は必ずアドレスを設定する
  nextExec.insert(EM_FIND, EM_FILES);           //EEPROM検知後はファイル一覧を検索する
  nextExec.insert(EM_WRITE, EM_FILES);          //EEPROM書き込み後はファイル一覧を検索する
  nextExec.insert(EM_TMP_WRITE, EM_FILES);      // 同上
  nextExec.insert(EM_DEL, EM_FILES);            //EEPROMファイル削除後はファイル一覧を検索する

  // 連鎖コマンド群（書込系）
  nextExec.insert(EM_INS_WRITE, EM_TMP_WRITE);  //更新・挿入書き込み後に退避したファイルを書き込む
  nextExec.insert(EM_INS_ADDR, EM_INS_WRITE);   //更新・挿入書き込みアドレス設定後に書き込みを行う
  nextExec.insert(EM_INS_READ, EM_INS_ADDR);    //更新・挿入書き込み前の退避読み込み後に書き込みアドレスを設定する
  nextExec.insert(EM_RES_ADDR, EM_INS_READ);    //更新書き込み前の退避アドレス設定後に退避読み込みを行う

  // 連鎖コマンド群（削除系）
  nextExec.insert(EM_DELW_ADDR, EM_TMP_WRITE);  //1件削除用アドレス再設定後に退避したファイルを書き込む
  nextExec.insert(EM_DEL_READ, EM_DELW_ADDR);   //1件削除前の読み込み後にアドレス再設定を行う
  nextExec.insert(EM_DELR_ADDR, EM_DEL_READ);   //1件削除前の退避アドレス設定後に読み込みを行う

  //シリアルポートの表示をリフレッシュ
  on_btnRefresh_clicked();
}

//-----------------------------
//デストラクタ
MainWindow::~MainWindow()
{
  //生成したオブジェクトを削除
  delete ui;
  delete serial;

  //サブプロセスが実行中の場合は停止後に削除
  Proc_Kill(nowProc);
  delete nowProc;
  Proc_Kill(nowEditor);
  delete nowEditor;

  //一時ファイルがオープンされていた場合はクローズして削除
  if (nowTmp.isOpen()) nowTmp.close();
  nowTmp.remove();
}

// Private Methods ====================================================================================================
// 文字列表示用 *****************************************************************
//-----------------------------
// 数値->16進数変換
// 　引数：数値
// 　戻値：16進数変換後の文字列
QString MainWindow::IntToHex(int data)
{
  QString buff;

  //上位1ケタを処理
  int tmp = data / 16;
  if (tmp < 10) buff.append('0' + (char)tmp);
  else buff.append('A' + (char)(tmp-10));

  //下位1ケタを処理
  data %= 16;
  if (data < 10) buff.append('0' + (char)data);
  else buff.append('A' + (char)(data-10));

  return buff;
}

//-----------------------------
// 数値2個->パーセント変換
// 　条件：残り時間表示のため、分子=0 の状態で初回呼び出しが必要
// 　引数：変換用分子、変換用分母
// 　戻値：パーセント＋残り時間の文字列
QString MainWindow::IntToPercent(int valNow, int valMax)
{
  QString buff;

  //進捗率を計算し、数字を補正
  int valPers = (valNow * 100) / valMax;
  if (valPers > 100) valPers = 100;

  //右詰して表示用文字列を作成
  if(valPers < 100) buff.append(' ');
  if(valPers < 10) buff.append(' ');
  buff.append(QString::number(valPers));
  buff.append('\x25');
  buff.append(' ');

  //初回呼び出しの場合はタイマーを開始
  if (valNow == 0) timer.start();
  //初回以外の場合は残り時間を計算して追加表示
  else if (valNow != valMax) {
    //経過時間と進捗率から残り時間を算出
    int remsec = (timer.elapsed() * (valMax - valNow) / valNow / 1000) + 1; // 残り0秒を防止するため1を加算
    //分と秒に変換して残り時間表示用文字列を作成
    buff.append("(remain ");
    if (remsec >= 60) {
      buff.append(QString::number(int(remsec / 60)));
      buff.append("min ");
    }
    buff.append(QString::number(int(remsec % 60)));
    buff.append("sec)");
  }

  return buff;
}

// ダイアログ *********************************************************************
//-----------------------------
// エラーメッセージ表示
// 　引数：表示するメッセージ文字列
void MainWindow::ErrorMessage(QString msg)
{
  QMessageBox msgBox(this);
  msgBox.setText(msg);
  msgBox.exec();
}

//-----------------------------
// Yes/No選択ダイアログ表示
// 　引数：表示するメッセージ文字列
// 　戻値：1=Yes, 0=No
int MainWindow::SelectYorC(QString msg)
{
  QMessageBox::StandardButton reply = QMessageBox::question(this, "Select", msg, QMessageBox::Yes|QMessageBox::Cancel);
  if(reply == QMessageBox::Yes) return 1;

  return 0;
}

// コマンド処理補助用 **************************************************************
//-----------------------------
// ライタへのコマンド発行
// 　引数：発行するコマンド
void MainWindow::StartCommand(execMode mode)
{
  QByteArray data;

  //終了の場合は後続処理は不要
  if (mode == EM_END) {
    nowExec = mode;
    nowLine = LM_CMD;
    CommandRunning(false);
    return;
  }
  //指定されたモードごとに、ライタ用コマンドを準備する
  switch(mode) {
    case EM_VER:          //バージョン表示（接続確認用）
      data.append('V');
      break;
    case EM_FIND:         //バンクサイズ取得（EEPROM確認用）
      data.append('F');
      break;
    case EM_FILES:        //ファイル一覧取得
      data.append('L');
      break;
    case EM_RES_ADDR:     //更新書き込み時のアドレス指定
    case EM_INS_ADDR:     //挿入書き込み時のアドレス指定
    case EM_DELR_ADDR:    //1件削除時の一時読み込み用アドレス指定
    case EM_DELW_ADDR:    //1件削除時の一時書き込み用アドレス指定
    case EM_ADDR:         //アドレス選択時のアドレス指定
      data.append('S');
      break;
    case EM_INS_WRITE:    //挿入書き込み時のHEX転送
    case EM_TMP_WRITE:    //退避ファイル書き込み時のHEX転送
    case EM_WRITE:        //単純書き込み時のHEX転送
      data.append('W');
      break;
    case EM_FUSE:         //Fuse設定
      data.append('Z');
      break;
    case EM_EEPROM:       //内蔵EEPROMの書き込み
      data.append('E');
      break;
    case EM_PROG:         //ファームウェアの書き込み
      data.append('P');
      break;
    case EM_DEL:          //単純削除
      data.append('C');
      break;
    case EM_INS_READ:     //挿入書き込み時のHEX受信
    case EM_DEL_READ:     //1件削除時のHEX受信
      data.append('R');
      break;
    default:
      return;
      break;
  }

  //現在のモードを更新
  nowExec = mode;

  //変数を初期化
  nowLine = LM_CMD;
  res_Line = "";
  res_Error = "";
  res_List.clear();

  //GUIを変更
  CommandRunning(true);

  //ライタにコマンドを発行
  serial->write(data);
}

//-----------------------------
// サブプロセスの強制停止
// 　引数：停止するコマンドへのポインタ
void MainWindow::Proc_Kill(QProcess* target)
{
  //対象プロセスが実行中だった場合、強制終了する
  if (target->state() == QProcess::Running) {
    target->kill();
    target->waitForFinished();
  }
}

//GUI補助用 ********************************************************************
//-----------------------------
// GUIコンソールへの出力
// 　引数：出力する文字列
void MainWindow::ConsoleOut(const QByteArray& buff)
{
  //テキストエリアの最後に追記する
  ui->lstConsole->moveCursor(QTextCursor::End);
  ui->lstConsole->insertPlainText(QString::fromLocal8Bit(buff));
  //最後の情報を表示する
  QScrollBar *bar = ui->lstConsole->verticalScrollBar();
  bar->setValue(bar->maximum());
}

//-----------------------------
// GUIコンソールの行戻し（行の先頭から再開）
void MainWindow::ConsoleReLine()
{
  //テキストエリアの最終行を削除する
  ui->lstConsole->moveCursor(QTextCursor::End);
  ui->lstConsole->moveCursor(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
  ui->lstConsole->cut();	//クリップボードが更新されてしまう
  //最後の情報を表示する
  QScrollBar *bar = ui->lstConsole->verticalScrollBar();
  bar->setValue(bar->maximum());
}

//-----------------------------
// GUIの未接続／接続済み状態制御
// 　引数：1=接続状態,0=未接続状態
void MainWindow::ConnectionSet(bool isConnected = 1)
{
  //ライタの接続状態に依存するコマンドを制御する
  nowConnect = isConnected;
  ui->btnFiles->setEnabled(isConnected);
  ui->btnFuse->setEnabled(isConnected);
  ui->btnElfWrite->setEnabled(isConnected);
  ui->btnEEPROMWrite->setEnabled(isConnected);

  //ライタの接続状態変更時はEEPROMは未接続の状態とする
  EEPROMExist(false);
}

//-----------------------------
// GUIのEEPROM存在／非存在状態制御
// 　引数：1=存在状態,0=非存在状態
void MainWindow::EEPROMExist(bool isExist = 1)
{
  //EEPROMの存在状態に依存するコマンドを制御する
  nowEEPROM = isExist;
  ui->btnAddr->setEnabled(isExist);
  ui->btnWrite->setEnabled(isExist);
  ui->btnDelete->setEnabled(isExist);
  ui->txtAddr->setEnabled(isExist);
}

//-----------------------------
// GUIのコマンド実行中／停止中状態制御
// 　引数：1=実行状態,0=停止状態
void MainWindow::CommandRunning(bool isRunning = 1)
{
  //ライタへのコマンド発行中に使用できないコマンドを制御する
  if (isRunning == true) {
    //コマンド発行時は、状態を一時保存し、非接続状態として扱う
    bool tmpConnect = nowConnect;
    bool tmpEEPROM = nowEEPROM;
    ConnectionSet(!isRunning);
    nowConnect = tmpConnect;
    nowEEPROM = tmpEEPROM;
  }
  else {
    //コマンド終了時は、一時保存した状態を復元する
    bool tmpEEPROM = nowEEPROM;
    ConnectionSet(nowConnect);
    nowEEPROM = tmpEEPROM;
    EEPROMExist(nowEEPROM);
  }
}

// SLOT Methods =======================================================================================================
// GUIイベント用の定義 *************************************************************
//-----------------------------
// (act:GUI)メニューから終了選択時の処理
void MainWindow::actExit()
{
  //画面をクローズする
  close();
}

//-----------------------------
// (act:GUI)メニューから設定選択時の処理
void MainWindow::actSettings()
{
  //設定用ダイアログを表示する
  setdialog->exec();
}

//-----------------------------
// (btn:GUI)リフレッシュボタンの制御
void MainWindow::on_btnRefresh_clicked()
{
  //GUIの状態を変更
  ConnectionSet(false);

  //関連するGUIを初期化
  ui->cmbPorts->clear();
  ui->lblVersion->clear();
  ui->lstFiles->clear();
  ui->lblBanks->setText("-------");

  //シリアルポート一覧を更新
  const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos) {
      ui->cmbPorts->addItem(info.portName());
  }
}

//-----------------------------
// (btn:GUI)ソースファイル参照ボタンの制御
void MainWindow::on_btnMML_clicked()
{
  //設定ファイルから過去のディレクトリを取得して表示（初期値はカレントディレクトリ）
  nowSettings->beginGroup("FOLDER");
  QString Folder = nowSettings->value("FOLDER_MML", "./").toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), Folder, tr("Source Files(*.mml *.par);;All Files(*.*)"));
  //値が指定されていた場合は設定ファイルに反映
  if (!fileName.isEmpty()) {
    nowSettings->setValue("FOLDER_MML", QFileInfo(fileName).absolutePath());
    ui->txtMML->setText(fileName);
  }
  nowSettings->endGroup();
  //設定ファイルを更新
  nowSettings->sync();
}

//-----------------------------
// (btn:GUI)バイナリファイル参照ボタンの制御
void MainWindow::on_btnBinary_clicked()
{
  //設定ファイルから過去のディレクトリを取得して表示（初期値はカレントディレクトリ）
  nowSettings->beginGroup("FOLDER");
  QString Folder = nowSettings->value("FOLDER_BIN", "./").toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), Folder, tr("Target Files(*.wav *.bin);;All Files(*.*)"));
  //値が指定されていた場合は設定ファイルに反映
  if (!fileName.isEmpty()) {
    nowSettings->setValue("FOLDER_BIN", QFileInfo(fileName).absolutePath());
    ui->txtBinary->setText(fileName);
  }
  nowSettings->endGroup();
  //設定ファイルを更新
  nowSettings->sync();
}

//-----------------------------
// (btn:GUI)EEPROMファイル参照ボタンの制御
void MainWindow::on_btnEEPROM_clicked()
{
  //設定ファイルから過去のディレクトリを取得して表示（初期値はカレントディレクトリ）
  nowSettings->beginGroup("FOLDER");
  QString Folder = nowSettings->value("FOLDER_EEPROM", "./").toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), Folder, tr("EEPROM Files (*.bin);;All Files(*.*)"));
  //値が指定されていた場合は設定ファイルに反映
  if (!fileName.isEmpty()) {
    nowSettings->setValue("FOLDER_EEPROM", QFileInfo(fileName).absolutePath());
    ui->txtEEPROM->setText(fileName);
  }
  nowSettings->endGroup();
  //設定ファイルを更新
  nowSettings->sync();
}

//-----------------------------
// (btn:GUI)ELFファイル参照ボタンの制御
void MainWindow::on_btnElf_clicked()
{
  //設定ファイルから過去のディレクトリを取得して表示（初期値はカレントディレクトリ）
  nowSettings->beginGroup("FOLDER");
  QString Folder = nowSettings->value("FOLDER_ELF", "./").toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), Folder, tr("ELF Files (*.elf);;All Files(*.*)"));
  //値が指定されていた場合は設定ファイルに反映
  if (!fileName.isEmpty()) {
    nowSettings->setValue("FOLDER_ELF", QFileInfo(fileName).absolutePath());
    ui->txtElf->setText(fileName);
  }
  nowSettings->endGroup();
  //設定ファイルを更新
  nowSettings->sync();
}

//-----------------------------
// (btn:GUI)Source to BIN実行ボタンの制御
void MainWindow::on_btnMML2BIN_clicked()
{
  QString fileName = ui->txtMML->text();
  QString toolName;
  QFileInfo fileInfo(fileName);

  //ファイルがエラーの場合は中止する
  if (fileName.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    ErrorMessage("File not exist!");
    return;
  }

  //設定ファイルから拡張子ごとのツールパスを取得する
  nowSettings->beginGroup("TOOLS");
  if (fileName.right(3).toUpper() == "MML") {
    toolName = nowSettings->value("MML2BIN", "MML2BIN.EXE").toString();
  } else {
    toolName = nowSettings->value("PAR2BIN", "PAR2BIN.EXE").toString();
  }
  nowSettings->endGroup();

  //コマンドを組み立てる
  QString cmdLine = "\"" + toolName + "\"";
  cmdLine += " \"" + fileName + "\"";

  //サブプロセスを初期化し、新たなプロセスを起動する
  Proc_Kill(nowProc);
  nowProc->start(cmdLine);
  if(!nowProc->waitForStarted())
    ErrorMessage(toolName + " tool not found.");
}

//-----------------------------
// (btn:GUI)Edit実行ボタンの制御
void MainWindow::on_btnMMLEdit_clicked()
{
  QString fileName = ui->txtMML->text();
  QFileInfo fileInfo(fileName);

  //ファイルがエラーの場合は中止する
  if (fileName.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    ErrorMessage("File not exist!");
    return;
  }

  //設定ファイルからエディタのパスを取得する
  nowSettings->beginGroup("TOOLS");
  QString cmdLine = "\"" + nowSettings->value("EDITOR", "notepad.exe").toString() + "\"";
  nowSettings->endGroup();

  //コマンドを組み立てる
  cmdLine += " \"" + fileName + "\"";

  //サブプロセスを初期化し、新たなプロセスを起動する
  Proc_Kill(nowEditor);
  nowEditor->start(cmdLine);
  if(!nowEditor->waitForStarted())
    ErrorMessage("EDITOR not found.");
}

//-----------------------------
// (btn:Terminal)接続ボタンの制御
void MainWindow::on_btnConnect_clicked()
{
  //シリアルポート接続中の場合はいったんクローズする
  if (serial->isOpen()) serial->close();

  //選択されたシリアルポートを指定する
  serial->setPortName(ui->cmbPorts->itemText(ui->cmbPorts->currentIndex()));

  //設定ファイルに従って接続設定を行う
  nowSettings->beginGroup("SERIAL");
  serial->setBaudRate(nowSettings->value("BAUD", QSerialPort::Baud115200).toInt());
  serial->setDataBits(QSerialPort::DataBits(nowSettings->value("BITS", QSerialPort::Data8).toInt()));
  serial->setParity(QSerialPort::Parity(nowSettings->value("PARITY", QSerialPort::NoParity).toInt()));
  serial->setStopBits(QSerialPort::StopBits(nowSettings->value("STOPBITS", QSerialPort::OneStop).toInt()));
  serial->setFlowControl(QSerialPort::FlowControl(nowSettings->value("FLOWCTRL", QSerialPort::NoFlowControl).toInt()));
  nowConsole = nowSettings->value("CONSOLELEVEL", 1).toInt();
  nowSettings->endGroup();

  //関連するGUIを初期化
  ui->lblVersion->clear();
  ui->lstFiles->clear();
  ui->lblBanks->setText("-------");

  //シリアルポートをオープンする
  serial->open(QIODevice::ReadWrite);

  //ライタにコマンドを発行
  //遷移：EM_VER -> EM_END
  StartCommand(EM_VER);
}

//-----------------------------
// (btn:Terminal)ファイル一覧ボタンの制御
void MainWindow::on_btnFiles_clicked()
{
  //コマンド実行中の場合は処理中断
  if (nowExec != EM_END) return;

  //関連するGUIを初期化
  ui->lstFiles->clear();
  ui->lblBanks->setText("-------");

  //ライタにコマンドを発行
  //遷移：EM_FIND -> EM_FILES -> EM_ADDR -> EM_END
  StartCommand(EM_FIND);
}

//-----------------------------
// (btn:Terminal)File書き込みボタンの制御
void MainWindow::on_btnWrite_clicked()
{
  execMode startMode;
  int lastSize;

  //コマンド実行中の場合は処理中断
  if (nowExec != EM_END) return;

  //ファイルがエラーの場合は中止する
  QString fileName = ui->txtBinary->text();
  QFileInfo fileInfo(fileName);
  if (fileName.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    ErrorMessage("File not exist!");
    return;
  }

  //確認のダイアログを表示する（キャンセルの場合は処理中断）
  QString msg = "Write Local File to EEPROM. Are you sure?";
  if (ui->cmbWriteMode->currentIndex() == 0) msg += "\n< Simple write >: Following data will be destroyed.";
  else if (ui->cmbWriteMode->currentIndex() == 1) msg += "\n< Resize write >: It takes a while.";
  else msg += "\n< Insert write >: It takes a while.";
  if (SelectYorC(msg) == 0) return;

  //ファイルがオープンできない場合は中止する
  if (nowFile.isOpen()) nowFile.close();
  nowFile.setFileName(fileName);
  if (!nowFile.open(QIODevice::ReadOnly)) {
    ErrorMessage("File cannot open!");
    return;
  }

  //単純書き込み（後続破壊）の場合（設定による場合と、リスト上の選択による場合）
  if (ui->cmbWriteMode->currentIndex() == 0 //選択による場合
       || ui->lstFiles->currentRow() >= ui->lstFiles->count() - 1 //リストの最後(END)が対象の場合
       || ( ui->cmbWriteMode->currentIndex() == 1 && ui->lstFiles->currentRow() >= ui->lstFiles->count() - 2) //最終項目をResizeの場合
      ) {
    //ライタに発行するコマンドを設定
    //遷移:EM_WRITE -> EM_FILES -> EM_ADDR -> EM_END
    startMode = EM_WRITE;
    //サイズオーバー判定用の残りサイズは、書き込み先アドレスから算出する
    lastSize = (nowBank * 0x10000 - 1) - QString("0x" + nowAddr).toInt(0, 16);
  }
  //後続を一時保存する書き込みの場合
  else {
    //書き込み先アドレスをいったん退避
    nextAddr = nowAddr;

    //更新書き込み（選択された項目への上書き）の場合
    if (ui->cmbWriteMode->currentIndex() == 1) {
      //一時保存の対象は、リストで選択された次の項目から
      nowSize =
          QString("0x" + ui->lstFiles->item(ui->lstFiles->count()-1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
          - QString("0x" + ui->lstFiles->item(ui->lstFiles->currentRow() + 1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
          +1;
      nowAddr = ui->lstFiles->item(ui->lstFiles->currentRow() + 1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6);
      //ライタに発行するコマンドを設定
      //遷移:EM_RES_ADDR -> EM_INS_READ -> EM_INS_ADDR -> EM_INS_WRITE -> EM_TMP_WRITE -> EM_FILES -> EM_ADDR -> EM_END
      startMode = EM_RES_ADDR;
      //進捗表示（読み込み）用の変数を設定する
      maxProgR = nowProgR = nowSize;
    //挿入書き込みの場合
    } else {
      //一時保存の対象は、リストで選択された項目から
      nowSize =
          QString("0x" + ui->lstFiles->item(ui->lstFiles->count()-1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
          - QString("0x" + ui->lstFiles->item(ui->lstFiles->currentRow())->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
          +1;
      //ライタに発行するコマンドを設定
      //遷移:EM_INS_READ -> EM_INS_ADDR -> EM_INS_WRITE -> EM_TMP_WRITE -> EM_FILES -> EM_ADDR -> EM_END
      startMode = EM_INS_READ;
      //進捗表示（読み込み）用の変数を設定する
      maxProgR = nowProgR = nowSize;
    }
    //サイズオーバー判定用の残りサイズは、書き込み先アドレス＋一時保存対象サイズから算出する
    lastSize = (nowBank * 0x10000 - 1) - (QString("0x" + nextAddr).toInt(0, 16) + nowSize);

    //一時保存用ファイルがオープンされていた場合はクローズし、オープン
    if (nowTmp.isOpen()) nowTmp.close();
    nowTmp.open();
  }

  //サイズオーバー判定を行う
  if (fileInfo.size() > lastSize) {
    ErrorMessage("Data exceed 0x" + QString::number(nowBank * 0x10000 - 1, 16));
    return;
  }

  //書き込みブロックサイズ、進捗表示用の変数を設定する
  nowBlock = 32;
  maxProg = nowProg = fileInfo.size();

  //ライタにコマンドを発行
  StartCommand(startMode);
}

//-----------------------------
// (btn:Terminal)File削除ボタンの制御
void MainWindow::on_btnDelete_clicked()
{
  //コマンド実行中の場合は処理中断
  if (nowExec != EM_END) return;

  //確認のダイアログを表示する（キャンセルの場合は処理中断）
  QString msg = "Delete File on EEPROM. Are you sure?";
  if (ui->cmbWriteMode->currentIndex() == 0) msg += "\n< Simple write >: Following data will be destroyed.";
  else  msg += "\n< Resize or Insert write >: It takes a while.";
  if (SelectYorC(msg) == 0) return;

  //単純削除（後続破壊）の場合
  if (ui->cmbWriteMode->currentIndex() == 0 || ui->lstFiles->currentRow() >= ui->lstFiles->count() - 2) {
    //ライタにコマンドを発行
    //遷移:EM_DEL -> EM_FILES -> EM_ADDR -> EM_END
    StartCommand(EM_DEL);
  }
  //1件削除（後続を詰める処理）の場合
  else {
    //書き込み先アドレスをいったん退避
    nextAddr = nowAddr;

    //一時保存の対象は、リストで選択された次の項目から
    nowSize =
        QString("0x" + ui->lstFiles->item(ui->lstFiles->count()-1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
        - QString("0x" + ui->lstFiles->item(ui->lstFiles->currentRow() + 1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
        +1;
    nowAddr = ui->lstFiles->item(ui->lstFiles->currentRow() + 1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6);

    //書き込みブロックサイズ、進捗表示（読み込み）用の変数を設定する
    nowBlock = 32;
    maxProgR = nowProgR = nowSize;

    //一時保存用ファイルがオープンされていた場合はクローズし、オープン
    if (nowTmp.isOpen()) nowTmp.close();
    nowTmp.open();

    //ライタにコマンドを発行
    //遷移:EM_DELR_ADDR -> EM_DEL_READ -> EM_DELW_ADDR -> EM_TMP_WRITE -> EM_FILES -> EM_ADDR -> EM_END
    StartCommand(EM_DELR_ADDR);
  }
}

//-----------------------------
// (btn:Terminal)内蔵EEPROM書き込みボタンの制御
void MainWindow::on_btnEEPROMWrite_clicked()
{
  QByteArray buff;

  //コマンド実行中の場合は処理中断
  if (nowExec != EM_END) return;

  //ファイルがエラーの場合は中止する
  QString fileName = ui->txtEEPROM->text();
  QFileInfo fileInfo(fileName);
  if (fileName.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    ErrorMessage("File not exist!");
    return;
  }

  //確認のダイアログを表示する（キャンセルの場合は処理中断）
  if (SelectYorC("Write EEPROM file to ATTiny85. ELF will be destroyed. Are you sure?") == 0) return;

  //ファイルがオープンできない場合は中止する
  if (nowFile.isOpen()) nowFile.close();
  nowFile.setFileName(fileName);
  if (!nowFile.open(QIODevice::ReadOnly)) {
    ErrorMessage("File cannot open!");
    return;
  }

  //書き込みブロックサイズ、応答用サイズ、進捗表示用の変数を設定する
  nowBlock = 64;
  nowProg = maxProg = nowSize = nowFile.size();

  //ライタにコマンドを発行
  //遷移:EM_EEPROM -> EM_END
  StartCommand(EM_EEPROM);
}

//-----------------------------
// (btn:Terminal)実行モジュール書き込みボタンの制御
void MainWindow::on_btnElfWrite_clicked()
{
  QByteArray buff;

  //コマンド実行中の場合は処理中断
  if (nowExec != EM_END) return;

  //ファイルがエラーの場合は中止する
  QString fileName = ui->txtElf->text();
  QFileInfo fileInfo(fileName);
  if (fileName.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    ErrorMessage("File not exist!");
    return;
  }

  //確認のダイアログを表示する（キャンセルの場合は処理中断）
  if (SelectYorC("Write ELF file to ATTiny85. Are you sure?") == 0) return;

  //ファイルがオープンできない場合は中止する
  if (nowFile.isOpen()) nowFile.close();
  nowFile.setFileName(fileName);
  if (!nowFile.open(QIODevice::ReadOnly)) {
    ErrorMessage("File cannot open!");
    return;
  }

  //ELFファイルの中身を解析し、プログラムの開始位置とサイズを得る
  // ELFヘッダをスキップ
  buff = nowFile.read(16);  // ".ELF" + e_ident
  buff = nowFile.read(2+2+4+4+4+4+4);
  buff = nowFile.read(2);   // ehsize
  buff = nowFile.read(2+2); // PHs * PHn
  buff = nowFile.read(2+2); // SHs * SHn
  buff = nowFile.read(2);   // SHT
  // プログラムヘッダテーブルから情報を得る
  buff = nowFile.read(4);   // Type
  buff = nowFile.read(4);   // File Offset
  //  プログラムの開始位置
  qint64 f_offset = (buff.at(0) & 0xFF) + ((buff.at(1) & 0xFF) << 8) + ((buff.at(2) & 0xFF) << 16) + ((buff.at(3) & 0xFF) << 24);
  buff = nowFile.read(4+4); // vaddr & paddr
  buff = nowFile.read(4);   // File Size
  //  プログラムのサイズ
  qint64 f_size = (buff.at(0) & 0xFF) + ((buff.at(1) & 0xFF) << 8) + ((buff.at(2) & 0xFF) << 16) + ((buff.at(3) & 0xFF) << 24);

  //プログラムの開始位置にシークする
  nowFile.seek(f_offset);

  //書き込みブロックサイズ、応答用サイズ、進捗表示用の変数を設定する
  nowBlock = 64;
  nowProg = maxProg = nowSize = (int)f_size;

  //ライタにコマンドを発行
  //遷移:EM_PROG -> EM_END
  StartCommand(EM_PROG);
}

//-----------------------------
// (btn:Terminal)Fuse変更実行ボタンの制御
void MainWindow::on_btnFuse_clicked()
{
  //コマンド実行中の場合は処理中断
  if (nowExec != EM_END) return;

  //確認のダイアログを表示する（キャンセルの場合は処理中断）
  if (SelectYorC("Fuse will be initialized. Are you sure?") == 0) return;

  //リストで選択されているFuseコマンドを保存する
  nowFuse = ui->cmbFuse->currentIndex();

  //ライタにコマンドを発行
  //遷移:EM_FUSE -> EM_END
  StartCommand(EM_FUSE);
}

//-----------------------------
// (lst:Terminal)Fileリスト選択確定時の処理
void MainWindow::on_lstFiles_currentRowChanged(int)
{
  //アドレス転送処理を呼び出す
  on_btnAddr_clicked();
}

//-----------------------------
// (btn:Terminal)アドレス転送（>>）ボタンの制御
void MainWindow::on_btnAddr_clicked()
{
  //コマンド実行中の場合は処理中断
  if (nowExec != EM_END) return;

  //リストの選択が異常な場合（リストが空の場合に該当）は処理中断
  if (ui->lstFiles->currentItem() == NULL) return;

  //リストからアドレスを得てテキストボックスに反映する
  ui->txtAddr->setText(ui->lstFiles->currentItem()->data(Qt::DisplayRole).toString().left(6));

  //編集完了のイベントを呼び出す
  on_txtAddr_editingFinished();
}

//-----------------------------
// (txt:Terminal)アドレス編集確定時の処理
void MainWindow::on_txtAddr_editingFinished()
{
  //コマンド実行中の場合は処理中断
  if (nowExec != EM_END) return;

  //テキストボックスに入力されているEEPROMアドレスを保存する
  nowAddr = ui->txtAddr->text();

  //ライタにコマンドを発行
  //遷移:EM_ADDR -> EM_END
  StartCommand(EM_ADDR);
}

// シグナルイベントの定義 ************************************************************
//-----------------------------
// サブプロセス出力内容受信時の処理
void MainWindow::on_ProcessOut()
{
  //コンソールに内容を出力する
  QByteArray buff = nowProc->readAllStandardOutput();
  ConsoleOut(buff);
}

//-----------------------------
// サブプロセス終了時の処理
void MainWindow::on_ProcessFinished(int code, QProcess::ExitStatus)
{
  //正常終了した場合、続けて書き込みできるよう、バイナリファイル名を更新する
  if (code == 0) {
    QRegExp reg("\\.(mml|par)$");
    reg.setCaseSensitivity(Qt::CaseInsensitive);
    ui->txtBinary->setText(ui->txtMML->text().replace(reg, ".BIN"));
  }
}

//-----------------------------
// シリアル通信受信時の処理
void MainWindow::readData()
{
  QByteArray text;

  //受信したデータを取得する
  QByteArray data = serial->readAll();

  //受信したデータをすべて処理する
  for (int i = 0; i < data.size(); i ++) {
    // 処理対象の一文字取り出す
    char chr = data.at(i);
    // 復帰は無視
    if (chr == '\r') {
      ;
    }
    // 1行応答モードの開始
    else if (chr == ':') {
      nowLine = LM_RES_1;
      res_Line = "";		//格納用変数の初期化
    }
    // リスト応答モードの開始
    else if (chr == '>') {
      nowLine = LM_LIST;
      res_List.append("");	//格納用配列に1行追加
    }
    // エラー応答モードの開始
    else if (chr == '!') {
      nowLine = LM_ERR;
      res_Error = "";
    }
    // コメント行モードの開始
    else if (chr == '#') {
      nowLine = LM_MESG;
    }
    // 結果行モードの開始
    else if (chr == '\'') {
      nowLine = LM_RES_L;
    }
    // 入力待ちモードに移行
    else if (chr == '?' || chr == ';') {
      QByteArray buff;
      //各種動作モードごとの処理を行う
      switch(nowExec) {
        //アドレス応答系の処理 =============================
        case EM_RES_ADDR:
        case EM_INS_ADDR:
        case EM_DELR_ADDR:
        case EM_DELW_ADDR:
        case EM_ADDR:
          //アドレスを入力する
          buff = nowAddr.toLocal8Bit();
          serial->write(buff);
          break;
        //定数応答系の処理 =================================
        case EM_DEL:
          //削除中を表示する
          text.append("Deleting... ");
          //更新値として '00' を入力する
          buff.append("00");
          serial->write(buff);
          break;
        //選択値応答系の処理================================
        case EM_FUSE:
          //書き込み中を表示する
          text.append("Fuse Writing... ");
          //リストの選択状態に応じた値を入力する
          if (nowFuse == 0) buff.append("FF");
          else if (nowFuse == 2) buff.append("22");
          else buff.append("11");
          serial->write(buff);
          break;
        //HEX読み込み系の処理 ==============================
        case EM_DEL_READ:
        case EM_INS_READ:
          if(chr == '?') {
            //開始処理の場合、読み込みサイズを入力する
            buff = (QString("000000") + QString::number(nowSize, 16)).right(6).toLocal8Bit();
            serial->write(buff);
          }
          else {
            //途中経過の場合、進捗率を表示する
            ConsoleReLine();
            text.append("TMP Reading... " + IntToPercent(maxProgR - nowProgR, maxProgR));
            nowProgR -= nowBlock; //リード時の1行サイズ
          }
          break;
        //HEX書き込み系の処理 ==============================
        //一時処理系固有 -----------------------------------
        case EM_TMP_WRITE:
          //コンソールに "TMP" を表示
          text.append("TMP ");
          //他の書き込みモードと同じ処理を行う
        //共通処理 -----------------------------------------
        case EM_WRITE:
        case EM_INS_WRITE:
        case EM_EEPROM:
        case EM_PROG:
          //進捗率を表示する
          text.append("Writing..." + IntToPercent(maxProg - nowProg, maxProg));
          if(chr == ';') {
            //途中経過の場合、コンソールレベルにより表示処理を分ける
            if (nowConsole >= 1) text.append('\n');
            else ConsoleReLine();
          }
          //現在のファイルがオープン状態の場合は処理を行う
          if (nowFile.isOpen()) {
            QByteArray tmpdata = nowFile.read(nowProg >= nowBlock ? nowBlock : nowProg);
            if (tmpdata.count() == 0 || nowProg == 0) {
              //ファイルが終端に達した場合はHEXファイルの終端を入力する
              buff.append(":00000001FF\n\r");
              nowFile.close();
            }
            else {
              //それ以外の場合はHEXファイルのデータを入力する
              nowProg -= tmpdata.count();
              buff.append(':');
              buff.append(IntToHex(tmpdata.count()));
              buff.append("000000");
              buff.append(tmpdata.toHex());
              buff.append("00\n\r");
            }
            serial->write(buff);
          }
          break;
      }
      nowLine = LM_CMD;
    }
    // 全コマンド終了後、コマンド待ちモードへの移行
    else if (chr == '@') {
      //各種動作モードごとの終了処理を行う
      switch(nowExec) {
        //バージョン表示モード終了時の処理 =================
        case EM_VER:
          //得られたバージョン情報をラベルに反映
          nowVer = res_Line;
          ui->lblVersion->setText(nowVer);
          //GUIの制御
          ConnectionSet(true);
          break;
        //バンクサイズ取得モード終了時の処理 ===============
        case EM_FIND:
          //応答行数＝バンク数として保存
          nowBank = res_List.count();
          //0行の応答だった場合はバンクサイズ無しを表示
          if (nowBank == 0) {
            ui->lblBanks->setText("-------");
            //GUIの制御
            EEPROMExist(false);
            //処理フローの終了
            nowExec = EM_ABORT;
          }
          //複数行の応答があった場合はバンクサイズを表示
          else {
            ui->lblBanks->setText(QString("0x") + QString::number(nowBank, 16) + QString("0000"));
          }
          break;
        //ファイル一覧取得モード終了時の処理 ===============
        case EM_FILES:
          //取得結果をGUIに反映
          ui->lstFiles->clear();
          for (int i = 0; i < res_List.count(); i++) ui->lstFiles->addItem(res_List.at(i));
          ui->lstFiles->setCurrentRow(ui->lstFiles->count()-1);
          //選択状態が異常の場合は処理中断
          if (ui->lstFiles->currentItem() == NULL) {
            nowExec = EM_ABORT;
          }
          //選択状態が正常な場合は後続処理を実行
          else {
            //リストからアドレスを得てテキストボックスに反映する
            nowAddr = ui->lstFiles->currentItem()->data(Qt::DisplayRole).toString().left(6);
            ui->txtAddr->setText(nowAddr);
            //GUIを制御
            EEPROMExist(true);
          }
          break;
        //OK応答のある終了処理 =============================
        //ファイルクローズの必要なモード -------------------
        case EM_TMP_WRITE:
        case EM_WRITE:
        case EM_INS_WRITE:
        case EM_EEPROM:
        case EM_PROG:
          //ファイルをクローズする
          if (nowFile.isOpen()) nowFile.close();
          //後続と同じ処理を行う
        //コンソール初期化が必要なモード -------------------
        case EM_DEL:
        case EM_FUSE:
          //コンソールの初期化
          ConsoleReLine();
          //後続と同じ処理を行う
        //共通処理 -----------------------------------------
        case EM_RES_ADDR:
        case EM_INS_ADDR:
        case EM_DELR_ADDR:
        case EM_DELW_ADDR:
        case EM_ADDR:
          //エラー処理が不要なら処理フローを終了
          if (res_Line != "OK") {
            ErrorMessage("Error Response!" + res_Line);
            nowExec = EM_ABORT;
          }
          break;
        //ファイル書き出しを伴う終了処理 ===================
        case EM_DEL_READ:
        case EM_INS_READ:
          //コンソールの初期化
          ConsoleReLine();
          //読み込んだ全データをファイルに書き出し
          for (int i = 0; i < res_List.count(); i++) {
            QByteArray tmpbuff;
            QString tmpstr = res_List.at(i).mid(7);
            for (int j = 0; j < tmpstr.count(); j += 2) {
              tmpbuff.append((char)("0x" + tmpstr.mid(j, 2)).toInt(0, 16));
            }
            nowTmp.write(tmpbuff);
          }
          nowTmp.close();
          //本来の書き込み先のアドレスを復帰する
          nowAddr = nextAddr;
          break;
        //中断モードの処理（エラー時の特殊モード） =========
        case EM_ABORT:
          //ファイルをクローズする
          if (nowFile.isOpen()) nowFile.close();
          //エラー処理を行う
          ErrorMessage(res_Error);
          //GUIを制御
          ConnectionSet(false);
          break;
      }
      //遷移先動作モードごとの開始前処理を行う
      switch(nextExec[nowExec]) {
        //一時ファイルの使用が必要なモードの開始処理 =======
        case EM_TMP_WRITE:
          //読み込み時のサイズを進捗率表示用の変数に設定する
          maxProg = nowProg = maxProgR;
          //ファイルがオープンされていた場合はクローズする
          if (nowFile.isOpen()) nowFile.close();
          //一時退避したファイルを開く
          nowFile.setFileName(nowTmp.fileName());
          if (!nowFile.open(QIODevice::ReadOnly)) {
            ErrorMessage("Temporary file cannot open!");
            nowExec = EM_ABORT;
          }
          break;
      }
      
      //フローの遷移
      StartCommand(nextExec[nowExec]);
    }
    // それ以外（改行の場合を含む）の場合は行の状態によってデータを格納
    else {
       //各種行状態ごとの処理を行う
       switch(nowLine) {
         //1行応答モード
         case LM_RES_1:
           if (chr != '\n') res_Line.append(chr);
           break;
         //リスト応答モード
         case LM_LIST:
          if (chr != '\n') res_List[res_List.count()-1].append(chr);
          break;
         //エラー応答モード
         case LM_ERR:
           if (chr != '\n') res_Error.append(chr);
           else if (nowExec != EM_END) nowExec = EM_ABORT;  //行終端で異常終了モードに移行しておく（これ以降処理を行わないため）
           break;
         //コメント行モード
         case LM_MESG:
           if (nowConsole >= 2) text.append(chr); //コンソールモードによって表示可否を制御
           break;
         //結果行モード
         case LM_RES_L:
           if (nowConsole >= 1) text.append(chr); //コンソールモードによって表示可否を制御
           break;
       }
    }
  }

  ConsoleOut(text);
}

