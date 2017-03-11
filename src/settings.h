#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QSerialPort>
#include <QSettings>

namespace Ui {
  class Settings;
}

class Settings : public QDialog
{
  Q_OBJECT

public:
  explicit Settings(QSettings *setting, QWidget *parent = 0);
  ~Settings();

private slots:
  void on_buttonBox_accepted();

  void on_buttonBox_rejected();

  void on_btnMML2BIN_clicked();

  void on_btnEditor_clicked();

private:
  Ui::Settings *ui;
  QSettings* nowSettings;

};

#endif // SETTINGS_H
