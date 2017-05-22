/*
 * main.cpp
 *
 * 概要：
 *  AVR_MB_Writer のメインモジュール
 *
 * 使用方法等：
 *  AVR_MB_Writer のプロジェクトに組み込んで使用する
 *
 * ライセンス：
 *  Copyright (c) 2017, Hiro OTSUKA All rights reserved.
 *  （同梱の license.md参照 / See license.md）
 *
 * 更新履歴：
 *  2017/05/21 正式版公開(Hiro OTSUKA)
 *
 */

#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>
#include <QSettings>

//=============================
//メイン関数
//  引数：引数の個数、引数へのポインタ配列
int main(int argc, char *argv[])
{
  // 設定ファイルを定義
  QSettings* aSetting = new QSettings("AVR_MB.ini", QSettings::IniFormat);
  // 日本語をSJISに設定
  QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());

  //各ウィンドウの起動
  QApplication a(argc, argv);
  MainWindow w(aSetting);
  w.show();

  int ret = a.exec();
  delete aSetting;
  return ret;
}
