#include "settings.h"
#include "ui_settings.h"
#include <QFileDialog>

Settings::Settings(QSettings* setting, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::Settings)
{
  ui->setupUi(this);
  nowSettings = setting;

  ui->cmbBaudRate->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
  ui->cmbBaudRate->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
  ui->cmbBaudRate->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
  ui->cmbBaudRate->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);

  ui->cmbDataBits->addItem(QStringLiteral("5"), QSerialPort::Data5);
  ui->cmbDataBits->addItem(QStringLiteral("6"), QSerialPort::Data6);
  ui->cmbDataBits->addItem(QStringLiteral("7"), QSerialPort::Data7);
  ui->cmbDataBits->addItem(QStringLiteral("8"), QSerialPort::Data8);

  ui->cmbParity->addItem(QStringLiteral("None"), QSerialPort::NoParity);
  ui->cmbParity->addItem(QStringLiteral("Even"), QSerialPort::EvenParity);
  ui->cmbParity->addItem(QStringLiteral("Odd"), QSerialPort::OddParity);
  ui->cmbParity->addItem(QStringLiteral("Mark"), QSerialPort::MarkParity);
  ui->cmbParity->addItem(QStringLiteral("Space"), QSerialPort::SpaceParity);

  ui->cmbStopBits->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_OS_WIN
  ui->cmbStopBits->addItem(QStringLiteral("1.5"), QSerialPort::OneAndHalfStop);
#endif

  ui->cmbFlowControl->addItem(tr("None"), QSerialPort::NoFlowControl);
  ui->cmbFlowControl->addItem(tr("RTS/CTS"), QSerialPort::HardwareControl);
  ui->cmbFlowControl->addItem(tr("XON/XOFF"), QSerialPort::SoftwareControl);

  ui->cmbConsoleLevel->addItem(tr("Silent"), 0);
  ui->cmbConsoleLevel->addItem(tr("Normal"), 1);
  ui->cmbConsoleLevel->addItem(tr("Debug"), 2);

  nowSettings->beginGroup("SERIAL");
  int setBaudRate = nowSettings->value("BAUD", QSerialPort::Baud9600).toInt();
  int setDataBits = nowSettings->value("BITS", QSerialPort::Data8).toInt();
  int setParity = nowSettings->value("PARITY", QSerialPort::NoParity).toInt();
  int setStopBits = nowSettings->value("STOPBITS", QSerialPort::OneStop).toInt();
  int setFlowControl = nowSettings->value("FLOWCTRL", QSerialPort::NoFlowControl).toInt();
  int setConsoleLevel = nowSettings->value("CONSOLELEVEL", 1).toInt();
  nowSettings->endGroup();

  for (int i = 0; i < ui->cmbBaudRate->count(); i ++) if (ui->cmbBaudRate->itemData(i).toInt() == setBaudRate) ui->cmbBaudRate->setCurrentIndex(i);
  for (int i = 0; i < ui->cmbDataBits->count(); i ++) if (ui->cmbDataBits->itemData(i).toInt() == setDataBits) ui->cmbDataBits->setCurrentIndex(i);
  for (int i = 0; i < ui->cmbParity->count(); i ++) if (ui->cmbParity->itemData(i).toInt() == setParity) ui->cmbParity->setCurrentIndex(i);
  for (int i = 0; i < ui->cmbStopBits->count(); i ++) if (ui->cmbStopBits->itemData(i).toInt() == setStopBits) ui->cmbStopBits->setCurrentIndex(i);
  for (int i = 0; i < ui->cmbFlowControl->count(); i ++) if (ui->cmbFlowControl->itemData(i).toInt() == setFlowControl) ui->cmbFlowControl->setCurrentIndex(i);
  for (int i = 0; i < ui->cmbConsoleLevel->count(); i ++) if (ui->cmbConsoleLevel->itemData(i).toInt() == setConsoleLevel) ui->cmbConsoleLevel->setCurrentIndex(i);

  nowSettings->beginGroup("TOOLS");
  ui->txtMML2BIN->setText(nowSettings->value("MML2BIN", "MML2BIN.EXE").toString());
  ui->txtPAR2BIN->setText(nowSettings->value("PAR2BIN", "PAR2BIN.EXE").toString());
  ui->txtEditor->setText(nowSettings->value("EDITOR", "notepad.exe").toString());
  nowSettings->endGroup();
}

Settings::~Settings()
{
  delete ui;
}

void Settings::on_buttonBox_accepted()
{
  nowSettings->beginGroup("SERIAL");
  nowSettings->setValue("BAUD", ui->cmbBaudRate->currentData().toInt());
  nowSettings->setValue("BITS", ui->cmbDataBits->currentData().toInt());
  nowSettings->setValue("PARITY", ui->cmbParity->currentData().toInt());
  nowSettings->setValue("STOPBITS", ui->cmbStopBits->currentData().toInt());
  nowSettings->setValue("FLOWCTRL", ui->cmbFlowControl->currentData().toInt());
  nowSettings->setValue("CONSOLELEVEL", ui->cmbConsoleLevel->currentData().toInt());
  nowSettings->endGroup();

  nowSettings->beginGroup("TOOLS");
  nowSettings->setValue("MML2BIN", ui->txtMML2BIN->text());
  nowSettings->setValue("PAR2BIN", ui->txtPAR2BIN->text());
  nowSettings->setValue("EDITOR", ui->txtEditor->text());
  nowSettings->endGroup();

  nowSettings->sync();
  close();
}

void Settings::on_buttonBox_rejected()
{
  close();
}

void Settings::on_btnMML2BIN_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), ui->txtMML2BIN->text(), tr("All Files(*.*)"));
  if(!fileName.isEmpty()) ui->txtMML2BIN->setText(fileName);
}

void Settings::on_btnPAR2BIN_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), ui->txtPAR2BIN->text(), tr("All Files(*.*)"));
  if(!fileName.isEmpty()) ui->txtPAR2BIN->setText(fileName);
}

void Settings::on_btnEditor_clicked()
{

  QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"), ui->txtEditor->text(), tr("All Files(*.*)"));
  if(!fileName.isEmpty()) ui->txtEditor->setText(fileName);
}

