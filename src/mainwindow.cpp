#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QScrollBar>
#include <QFileDialog>
#include <QTextCodec>

// コンストラクタ・デストラクタ =====================================================================================================
MainWindow::MainWindow(QSettings* setting, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setdialog = new Settings(setting, this);
  serial = new QSerialPort(this);
  connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
  nowExec = EM_END;
  nowLine = LM_NONE;
  nowProc = new QProcess(this);
  nowEditor = new QProcess(this);
  nowSettings = setting;
  connect(nowProc, SIGNAL(readyReadStandardOutput()), this, SLOT(on_ProcessOut()));
  connect(nowProc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(on_ProcessFinished(int, QProcess::ExitStatus)));
  connect(ui->actExit, &QAction::triggered, this, &MainWindow::actExit);
  connect(ui->actSettings, &QAction::triggered, this, &MainWindow::actSettings);
  on_btnRefresh_clicked();
  nowTmp.setAutoRemove(false);
}

MainWindow::~MainWindow()
{
  delete ui;
  delete serial;
  Proc_Kill(nowProc);
  delete nowProc;
  Proc_Kill(nowEditor);
  delete nowEditor;
  if (nowTmp.isOpen()) nowTmp.close();
  nowTmp.remove();
}

// Private Methods ====================================================================================================
// common ---------------------------------------------------------------------
QString MainWindow::IntToHex(int data)
{
  QString buff;

  int tmp = data / 16;
  if (tmp < 10) buff.append('0' + (char)tmp);
  else buff.append('A' + (char)(tmp-10));
  data %= 16;
  if (data < 10) buff.append('0' + (char)data);
  else buff.append('A' + (char)(data-10));

  return buff;
}

// commands --------------------------------------------------------------------
void MainWindow::StartCommand(execMode mode)
{
  QByteArray data;
  switch(mode) {
    case EM_VER:
      data.append('V');
      break;
    case EM_FIND:
      data.append('F');
      break;
    case EM_FILES:
      data.append('L');
      break;
    case EM_RES_ADDR:
    case EM_INS_ADDR:
    case EM_DEL_ADDR:
    case EM_DEL_ADDR2:
    case EM_ADDR:
      data.append('S');
      break;
    case EM_INS_WRITE:
    case EM_WRITE:
      data.append('W');
      break;
    case EM_FUSE:
      data.append('Z');
      break;
    case EM_PROG:
      data.append('P');
      break;
    case EM_DEL:
      data.append('C');
      break;
    case EM_INS_READ:
    case EM_DEL_READ:
      data.append('R');
      break;
    default:
      return;
      break;
  }
  nowExec = mode;
  nowLine = LM_CMD;
  res_Line = "";
  res_Error = "";
  res_List.clear();
  CommandRunning(true);
  serial->write(data);
}

// Process --------------------------------------------------------------------
void MainWindow::Proc_Kill(QProcess* target)
{
  if (target->state() == QProcess::Running) {
    target->kill();
    target->waitForFinished();
  }
}

// GUI ----------------------------------------------------------------------
void MainWindow::ErrorMessage(QString msg)
{
  QMessageBox msgBox(this);
  msgBox.setText(msg);
  msgBox.exec();
}

int MainWindow::SelectYorC(QString msg)
{
  QMessageBox::StandardButton reply = QMessageBox::question(this, "Select", msg, QMessageBox::Yes|QMessageBox::Cancel);
  if(reply == QMessageBox::Yes) return 1;

  return 0;
}

void MainWindow::ConsoleOut(const QByteArray& buff)
{
  ui->lstConsole->moveCursor(QTextCursor::End);
  ui->lstConsole->insertPlainText(QString::fromLocal8Bit(buff));
  QScrollBar *bar = ui->lstConsole->verticalScrollBar();
  bar->setValue(bar->maximum());
}

void MainWindow::ConnectionSet(bool isConnected = 1)
{
  nowConnect = isConnected;
  ui->btnFiles->setEnabled(isConnected);
  ui->btnFuse->setEnabled(isConnected);
  ui->btnElfWrite->setEnabled(isConnected);

  EEPROMExist(false);
}

void MainWindow::EEPROMExist(bool isExist = 1)
{
  nowEEPROM = isExist;
  ui->btnAddr->setEnabled(isExist);
  ui->btnWrite->setEnabled(isExist);
  ui->btnDelete->setEnabled(isExist);
  ui->txtAddr->setEnabled(isExist);
}

void MainWindow::CommandRunning(bool isRunning = 1)
{
  if (isRunning == true) {
    bool tmpConnect = nowConnect;
    bool tmpEEPROM = nowEEPROM;
    ConnectionSet(!isRunning);
    nowConnect = tmpConnect;
    nowEEPROM = tmpEEPROM;
  }
  else {
    bool tmpEEPROM = nowEEPROM;
    ConnectionSet(nowConnect);
    nowEEPROM = tmpEEPROM;
    EEPROMExist(nowEEPROM);
  }
}

// SLOT Methods =======================================================================================================
// btn(GUI Only) ------------------------------------------------------------------------
void MainWindow::on_btnRefresh_clicked()
{
  ConnectionSet(false);
  const auto infos = QSerialPortInfo::availablePorts();
  ui->cmbPorts->clear();

  for (const QSerialPortInfo &info : infos) {
      ui->cmbPorts->addItem(info.portName());
  }
}

void MainWindow::on_btnBinary_clicked()
{
  nowSettings->beginGroup("FOLDER");
  QString Folder = nowSettings->value("FOLDER_BIN", "./").toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), Folder, tr("Target Files(*.wav *.bin);;All Files(*.*)"));
  if (!fileName.isEmpty()) {
    nowSettings->setValue("FOLDER_BIN", QFileInfo(fileName).absolutePath());
    ui->txtBinary->setText(fileName);
  }
  nowSettings->endGroup();
  nowSettings->sync();
}

void MainWindow::on_btnElf_clicked()
{
  nowSettings->beginGroup("FOLDER");
  QString Folder = nowSettings->value("FOLDER_ELF", "./").toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), Folder, tr("ELF Files (*.elf);;All Files(*.*)"));
  if (!fileName.isEmpty()) {
    nowSettings->setValue("FOLDER_ELF", QFileInfo(fileName).absolutePath());
    ui->txtElf->setText(fileName);
  }
  nowSettings->endGroup();
  nowSettings->sync();
}

void MainWindow::on_btnMML_clicked()
{
  nowSettings->beginGroup("FOLDER");
  QString Folder = nowSettings->value("FOLDER_MML", "./").toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), Folder, tr("MML Files(*.mml);;All Files(*.*)"));
  if (!fileName.isEmpty()) {
    nowSettings->setValue("FOLDER_MML", QFileInfo(fileName).absolutePath());
    ui->txtMML->setText(fileName);
  }
  nowSettings->endGroup();
  nowSettings->sync();
}

void MainWindow::on_btnMML2BIN_clicked()
{
  QString fileName = ui->txtMML->text();
  QFileInfo fileInfo(fileName);

  nowSettings->beginGroup("TOOLS");
  QString cmdLine = "\"" + nowSettings->value("MML2BIN", "MML2BIN.EXE").toString() + "\"";
  nowSettings->endGroup();

  cmdLine += " \"" + fileName + "\"";

  if (fileName.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    ErrorMessage("File not exist!");
    return;
  }

  Proc_Kill(nowProc);
  nowProc->start(cmdLine);
  if(!nowProc->waitForStarted())
    ErrorMessage("MML2BIN tool not found.");
}

void MainWindow::on_btnMMLEdit_clicked()
{
  QString fileName = ui->txtMML->text();
  QFileInfo fileInfo(fileName);

  nowSettings->beginGroup("TOOLS");
  QString cmdLine = "\"" + nowSettings->value("EDITOR", "notepad.exe").toString() + "\"";
  nowSettings->endGroup();

  cmdLine += " \"" + fileName + "\"";

  if (fileName.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    ErrorMessage("File not exist!");
    return;
  }

  Proc_Kill(nowEditor);
  nowEditor->start(cmdLine);
  if(!nowEditor->waitForStarted())
    ErrorMessage("EDITOR not found.");
}

// btn(With Terminal) ------------------------------------------------------------------------
void MainWindow::on_btnConnect_clicked()
{
  if (serial->isOpen()) serial->close();
  serial->setPortName(ui->cmbPorts->itemText(ui->cmbPorts->currentIndex()));
  nowSettings->beginGroup("SERIAL");
  serial->setBaudRate(nowSettings->value("BAUD", QSerialPort::Baud115200).toInt());
  serial->setDataBits(QSerialPort::DataBits(nowSettings->value("BITS", QSerialPort::Data8).toInt()));
  serial->setParity(QSerialPort::Parity(nowSettings->value("PARITY", QSerialPort::NoParity).toInt()));
  serial->setStopBits(QSerialPort::StopBits(nowSettings->value("STOPBITS", QSerialPort::OneStop).toInt()));
  serial->setFlowControl(QSerialPort::FlowControl(nowSettings->value("FLOWCTRL", QSerialPort::NoFlowControl).toInt()));
  nowSettings->endGroup();
  serial->open(QIODevice::ReadWrite);

  StartCommand(EM_VER);
}

void MainWindow::on_btnFiles_clicked()
{
  if (nowExec != EM_END) return;
  StartCommand(EM_FIND);
}

void MainWindow::on_btnAddr_clicked()
{
  if (nowExec != EM_END) return;
  nowAddr = ui->lstFiles->currentItem()->data(Qt::DisplayRole).toString().left(6);
  ui->txtAddr->setText(nowAddr);
  on_txtAddr_editingFinished();
}

void MainWindow::on_btnWrite_clicked()
{
  execMode startMode;
  int lastSize;

  if (nowExec != EM_END) return;
  QString fileName = ui->txtBinary->text();
  QFileInfo fileInfo(fileName);
  if (fileName.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    ErrorMessage("File not exist!");
    return;
  }
  if (nowFile.isOpen()) nowFile.close();
  QString msg = "Write Local File to EEPROM. Are you sure?";
  if (ui->cmbWriteMode->currentIndex() == 0) msg += "\n< Simple write >: Following data will be destroyed.";
  else if (ui->cmbWriteMode->currentIndex() == 1) msg += "\n< Resize write >: It takes a while.";
  else msg += "\n< Insert write >: It takes a while.";
  if (SelectYorC(msg) == 0) return;

  nowFile.setFileName(fileName);
  if (!nowFile.open(QIODevice::ReadOnly)) {
    ErrorMessage("File cannot open!");
    return;
  }

  if (ui->cmbWriteMode->currentIndex() == 0 || ui->lstFiles->currentRow() >= ui->lstFiles->count() - 1) {
    startMode = EM_WRITE;
    lastSize = (nowBank * 0x10000 - 1) - QString("0x" + nowAddr).toInt(0, 16);
  }
  else {
    if (nowTmp.isOpen()) nowTmp.close();
    nextAddr = nowAddr;
    if (ui->cmbWriteMode->currentIndex() == 1) {
      nowSize =
          QString("0x" + ui->lstFiles->item(ui->lstFiles->count()-1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
          - QString("0x" + ui->lstFiles->item(ui->lstFiles->currentRow() + 1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
          +1;
      nowAddr = ui->lstFiles->item(ui->lstFiles->currentRow() + 1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6);
      startMode = EM_RES_ADDR;
    } else {
      nowSize =
          QString("0x" + ui->lstFiles->item(ui->lstFiles->count()-1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
          - QString("0x" + ui->lstFiles->item(ui->lstFiles->currentRow())->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
          +1;
      startMode = EM_INS_READ;
    }
    lastSize = (nowBank * 0x10000 - 1) - (QString("0x" + nextAddr).toInt(0, 16) + nowSize);
    nowTmp.open();
  }
  if (fileInfo.size() > lastSize) {
    ErrorMessage("Data exceed 0x" + QString::number(nowBank * 0x10000 - 1, 16));
    return;
  }
  StartCommand(startMode);
}

void MainWindow::on_btnDelete_clicked()
{
  if (nowExec != EM_END) return;
  QString msg = "Delete File on EEPROM. Are you sure?";
  if (ui->cmbWriteMode->currentIndex() == 0) msg += "\n< Simple write >: Following data will be destroyed.";
  else  msg += "\n< Resize or Insert write >: It takes a while.";
  if (SelectYorC(msg) == 0) return;

  if (ui->cmbWriteMode->currentIndex() == 0 || ui->lstFiles->currentRow() >= ui->lstFiles->count() - 2) StartCommand(EM_DEL);
  else {
    if (nowTmp.isOpen()) nowTmp.close();
    nowSize =
        QString("0x" + ui->lstFiles->item(ui->lstFiles->count()-1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
        - QString("0x" + ui->lstFiles->item(ui->lstFiles->currentRow() + 1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6)).toInt(0, 16)
        +1;
    nowTmp.open();
    nextAddr = nowAddr;
    nowAddr = ui->lstFiles->item(ui->lstFiles->currentRow() + 1)->data(Qt::ItemDataRole::DisplayRole).toString().left(6);
    StartCommand(EM_DEL_ADDR);
  }
}

void MainWindow::on_btnFuse_clicked()
{
  if (nowExec != EM_END) return;
  if (SelectYorC("Fuse will be initialized. Are you sure?") == 0) return;

  nowFuse = ui->cmbFuse->currentIndex();
  StartCommand(EM_FUSE);
}

void MainWindow::on_btnElfWrite_clicked()
{
  QByteArray buff;

  if (nowExec != EM_END) return;
  QString fileName = ui->txtElf->text();
  QFileInfo fileInfo(fileName);
  if (fileName.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    ErrorMessage("File not exist!");
    return;
  }
  if (nowFile.isOpen()) nowFile.close();
  if (SelectYorC("Write ELF file to ATTiny85. Are you sure?") == 0) return;

  nowFile.setFileName(fileName);
  if (!nowFile.open(QIODevice::ReadOnly)) {
    ErrorMessage("File cannot open!");
    return;
  }
  //Skip the header
  // ELF Header
  buff = nowFile.read(16);  // ".ELF" + e_ident
  buff = nowFile.read(2+2+4+4+4+4+4);
  buff = nowFile.read(2);   // ehsize
  buff = nowFile.read(2+2); // PHs * PHn
  buff = nowFile.read(2+2); // SHs * SHn
  buff = nowFile.read(2);   // SHT
  // Program Header table
  buff = nowFile.read(4);   // Type
  buff = nowFile.read(4);   // File Offset
  qint64 f_offset = (buff.at(0) & 0xFF) + ((buff.at(1) & 0xFF) << 8) + ((buff.at(2) & 0xFF) << 16) + ((buff.at(3) & 0xFF) << 24);
  buff = nowFile.read(4+4); // vaddr & paddr
  buff = nowFile.read(4);   // File Size
  qint64 f_size = (buff.at(0) & 0xFF) + ((buff.at(1) & 0xFF) << 8) + ((buff.at(2) & 0xFF) << 16) + ((buff.at(3) & 0xFF) << 24);
  nowSize = (int)f_size;
  nowFile.seek(f_offset);
  StartCommand(EM_PROG);
}

// txt ------------------------------------------------------------------------
void MainWindow::on_txtAddr_editingFinished()
{
  if (nowExec != EM_END) return;
  nowAddr = ui->txtAddr->text();
  StartCommand(EM_ADDR);
}

// lst ------------------------------------------------------------------------
void MainWindow::on_lstFiles_currentRowChanged(int)
{
  on_btnAddr_clicked();
}

// act(menu) ------------------------------------------------------------------
void MainWindow::actExit()
{
  close();
}

void MainWindow::actSettings()
{
  setdialog->exec();
}

// SIGNAL Methods =====================================================================================================
void MainWindow::on_ProcessOut()
{
  QByteArray buff = nowProc->readAllStandardOutput();
  ConsoleOut(buff);
}

void MainWindow::on_ProcessFinished(int code, QProcess::ExitStatus)
{
  if (code == 0) {
    QRegExp reg("\\.mml$");
    reg.setCaseSensitivity(Qt::CaseInsensitive);
    ui->txtBinary->setText(ui->txtMML->text().replace(reg, ".BIN"));
  }
}

void MainWindow::readData()
{
  QByteArray data = serial->readAll();
  QByteArray text;

  for (int i = 0; i < data.size(); i ++) {
    char chr = data.at(i);
    // 復帰は無視
    if (chr == '\r') {
      ;
    }
    // 1レス行モードの開始
    else if (chr == ':') {
      nowLine = LM_RES_1;
      res_Line = "";
    }
    // リストモードの開始
    else if (chr == '>') {
      nowLine = LM_LIST;
      res_List.append("");
    }
    // コメント行モードの開始
    else if (chr == '#') {
      nowLine = LM_MESG;
    }
    // 結果行モードの開始
    else if (chr == '\'') {
      nowLine = LM_RES_L;
    }
    // エラー行モードの開始
    else if (chr == '!') {
      nowLine = LM_ERR;
    }
    // 入力待ちへの対応
    else if (chr == '?' || chr == ';') {
      QByteArray buff;
      switch(nowExec) {
        case EM_RES_ADDR:
        case EM_INS_ADDR:
        case EM_DEL_ADDR:
        case EM_DEL_ADDR2:
        case EM_ADDR:
          buff = nowAddr.toLocal8Bit();
          serial->write(buff);
          break;
        case EM_DEL_READ:
        case EM_INS_READ:
          buff = (QString("000000") + QString::number(nowSize, 16)).right(6).toLocal8Bit();
          serial->write(buff);
          break;
        case EM_DEL:
          buff.append("00");
          serial->write(buff);
          break;
        case EM_INS_WRITE:
        case EM_WRITE:
          if(chr == ';') text.append('\n');
          if (nowFile.isOpen()) {
            QByteArray tmpdata = nowFile.read(32);
            if (tmpdata.count() == 0) {
              buff.append(":00000001FF\n\r");
              nowFile.close();
            }
            else {
              buff.append(':');
              buff.append(IntToHex(tmpdata.count()));
              buff.append("000000");
              buff.append(tmpdata.toHex());
              buff.append("00\n\r");
            }
            serial->write(buff);
          }
          break;
        case EM_PROG:
          if(chr == ';') text.append('\n');
          if (nowFile.isOpen()) {
            QByteArray tmpdata = nowFile.read(nowSize >= 64 ? 64 : nowSize);
            if (nowSize == 0) {
              buff.append(":00000001FF\n\r");
              nowFile.close();
            }
            else {
              nowSize -= tmpdata.count();
              buff.append(':');
              buff.append(IntToHex(tmpdata.count()));
              buff.append("000000");
              buff.append(tmpdata.toHex());
              buff.append("00\n\r");
            }
            serial->write(buff);
          }
          break;
        case EM_FUSE:
          if (nowFuse == 0) buff.append("FF");
          else if (nowFuse == 2) buff.append("22");
          else buff.append("11");
          serial->write(buff);
          break;
      }
      nowLine = LM_CMD;
    }
    // 全コマンドの終了、コマンド待ちへの移行
    else if (chr == '@') {
      switch(nowExec) {
        case EM_VER:
          nowVer = res_Line;
          ui->lblVersion->setText(nowVer);
          nowExec = EM_END;
//          StartCommand(EM_FIND);
          ConnectionSet(true);
          break;
        case EM_FIND:
          if (res_List.count() != 0) {
            nowBank = res_List.count();
            ui->lblBanks->setText(QString("0x") + QString::number(nowBank, 16) + QString("0000"));
            StartCommand(EM_FILES);
          }
          else {
            ui->lblBanks->setText("-------");
            nowExec = EM_END;
            EEPROMExist(false);
          }
          break;
        case EM_DEL:
          if (res_Line != "OK") {
            ErrorMessage("Error Response!");
          }
          nowExec = EM_END;
          StartCommand(EM_FILES);
          break;
        case EM_FILES:
          ui->lstFiles->clear();
          for (int i = 0; i < res_List.count(); i++) ui->lstFiles->addItem(res_List.at(i));
          ui->lstFiles->setCurrentRow(ui->lstFiles->count()-1);
          nowExec = EM_END;
          on_btnAddr_clicked();
          EEPROMExist(true);
          break;
        case EM_ADDR:
        case EM_FUSE:
          if (res_Line != "OK") {
            ErrorMessage("Error Response!" + res_Line);
          }
          nowExec = EM_END;
          break;
        case EM_WRITE:
          if (nowFile.isOpen()) nowFile.close();
          if (res_Line != "OK") {
            ErrorMessage("Error Response!" + res_Line);
            nowExec = EM_END;
          }
          else {
            nowExec = EM_END;
            StartCommand(EM_FILES);
          }
          break;
        case EM_PROG:
          if (nowFile.isOpen()) nowFile.close();
          if (res_Line != "OK") {
            ErrorMessage("Error Response!" + res_Line);
          }
          nowExec = EM_END;
          break;
        case EM_ABORT:
          if (nowFile.isOpen()) nowFile.close();
          ErrorMessage(res_Error);
          nowExec = EM_END;
          break;
        case EM_DEL_ADDR:
          nowExec = EM_END;
          StartCommand(EM_DEL_READ);
          break;
        case EM_RES_ADDR:
          nowExec = EM_END;
          StartCommand(EM_INS_READ);
          break;
        case EM_INS_ADDR:
          nowExec = EM_END;
          StartCommand(EM_INS_WRITE);
          break;
        case EM_DEL_READ:
        case EM_INS_READ:
          for (int i = 0; i < res_List.count() && nowSize > 0; i++) {
            QByteArray tmpbuff;
            QString tmpstr = res_List.at(i).mid(7);
            for (int j = 0; j < tmpstr.count(); j += 2) {
              tmpbuff.append((char)("0x" + tmpstr.mid(j, 2)).toInt(0, 16));
              nowSize --;
            }
            nowTmp.write(tmpbuff);
          }
          nowTmp.close();
          nowAddr = nextAddr;
          if(nowExec == EM_INS_READ) {
            nowExec = EM_END;
            StartCommand(EM_INS_ADDR);
          }
          else {
            nowExec = EM_END;
            StartCommand(EM_DEL_ADDR2);
          }
          break;
        case EM_INS_WRITE:
        case EM_DEL_ADDR2:
          nowExec = EM_END;
          if (nowFile.isOpen()) nowFile.close();
          nowFile.setFileName(nowTmp.fileName());
          if (!nowFile.open(QIODevice::ReadOnly)) {
            ErrorMessage("Temporary file cannot open!");
            break;
          }
          StartCommand(EM_WRITE);
          break;
      }
      nowLine = LM_CMD;
      if(nowExec == EM_END) CommandRunning(false);
    }
    // それ以外（改行の場合を含む）の場合はモードによってデータを格納
    else {
       switch(nowLine) {
         case LM_RES_1:
           if (chr != '\n') res_Line.append(chr);
           break;
         case LM_ERR:
           if (chr != '\n') res_Error.append(chr);
           else nowExec = EM_ABORT;
           break;
         case LM_LIST:
           if (chr != '\n') res_List[res_List.count()-1].append(chr);
           break;
//         case LM_MESG:
         case LM_RES_L:
           text.append(chr);
           break;
       }
    }
  }

  ConsoleOut(text);
}

