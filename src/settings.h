/*
 * settings.h
 *
 * 概要：
 *  settings.cpp のための定義
 *
 * 使用方法等：
 *  settings.cpp を使用するモジュールから include
 *
 * ライセンス：
 *  Copyright (c) 2017, Hiro OTSUKA All rights reserved.
 *  （同梱の license.md参照 / See license.md）
 *
 * 更新履歴：
 *  2017/05/21 正式版公開(Hiro OTSUKA)
 *
 */

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
  //GUIイベント用の定義
  void on_buttonBox_accepted();
  void on_buttonBox_rejected();
  void on_btnMML2BIN_clicked();
  void on_btnEditor_clicked();
  void on_btnPAR2BIN_clicked();

private:
  //内部オブジェクト
  Ui::Settings *ui;
  QSettings* nowSettings;	//iniファイル管理用オブジェクト

};

#endif // SETTINGS_H
