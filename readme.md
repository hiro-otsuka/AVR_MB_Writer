# AVR_MB_Writer
このプログラムは、[AVR_MusicBox](https://github.com/hiro-otsuka/AVR_MusicBox/) 用のEEPROM＆実行モジュール用ライタGUIです。
AVR_MusicBox とともに公開している [AVR_MB_Writer.ino](https://github.com/hiro-otsuka/AVR_MusicBox/blob/master/tools/AVR_MB_Writer.ino)
にシリアルポート経由で接続して使用することにより、簡単なGUI操作で、EEPROMへのファイル書き込み、削除およびATTiny85のFuse設定、実行ファイル書き込みを実行できます。

# 作者
(c) 2017 大塚ヒロ <otsuk atmark purple.plala.or.jp>  
ライセンスについては [LICENSE.md](/license.md) を参照。

# 使用条件
バイナリファイルを使用する場合は、Windows 32bit または 64bit 版が必要です。
ソースコードからコンパイルする場合は、Qt でコンパイルできる環境が前提となります。

# 使用方法
各コマンド（ボタン等）の説明は以下の通りです。

* Refresh  
 Serialポートの一覧を更新します。
 表示されたSerialポート一覧から接続先を選択できます。

* Connect  
 選択したSerialポートに接続します。

* Files  
 接続したSerialポートに接続されている AVR_MusicBoxの EEPROM 容量と、ファイルの一覧を取得し、リストに表示します。

* Source:  
 BINファイルのもとになる MML ファイルや PAR ファイルを選択します。

* Source to BIN  
 MML2BINツールやPAR2BINツールを使って、Sourceファイルをバイナリに変換します。
 エラーや実行結果は、ウィンドウ下部のコンソールに表示されます。
 ツールのPATHは、File -> Settings メニュー から設定できます。
 バイナリへの変換に成功すると、自動的にバイナリファイル名が Local File: に入力されます。

* Edit  
 MMLファイルをエディタで開きます。
 使用するエディタは、File -> Settings メニュー から設定できます。

* Local File:  
 EEPROMに書き込みを行うファイルを選択します。
 標準では、WAVファイルまたはBINファイルが指定可能です。

* \>\>  
 ファイルの一覧で選択されているEEPROMアドレスを、Addr: に入力します。
 リストの項目を選択することで Addr: に自動入力されるので、通常は使用する必要はありません。

* Addr:  
 Write や Delete の操作を行う対象アドレスです。
 ファイルの一覧からファイルを選択することで自動的に更新されるため、通常は修正する必要はありません。

* Write!(I2C EEPROM用)  
 Local File: に指定したファイルを EEPROM に書き込みます。

* Delete!  
 ファイル一覧で選択したファイルを削除します。

* Mode  
 EEROM に対する Write または Delete のモードを選択します。
  Simple Write ... 単純な書き込みです。後続ファイルを破壊します。
  Resize Write ... 選択中のファイルを変更します。後続ファイルは保持されます。
  Insert Write ... 選択中の位置にファイルを挿入します。後続ファイルは保持されます。
 なお、Delete操作では、Resize, Insert で動作の違いはありません（後続ファイルは保持されます）。

* ATTiny EEPROM:  
 ATTiny85 の EEPROM に書き込む設定ファイル（バイナリファイル）を選択します。

* Write!(AVR EEPROM用)  
 ATTiny EEPROM: に指定したファイルを ATTiny85 の EEPROM に書き込みます。

* ATTiny ELF:  
 ATTiny85 に書き込む ELFファイル（実行モジュール）を選択します。

* Write!(ELF用)  
 ATTiny ELF: に指定したファイルを ATTiny85 のフラッシュメモリに書き込みます。

* ATTiny85 Fuse:  
 ATTiny85 に書き込む Fuse 設定を選択します。
  Initialize ...ATTiny85に対し、AVR_MusicBox に必要な標準 Fuse を設定します。
  RST Enable ... RSTピンを有効（I/Oとして使用不可）に設定します。
  RST Disable ... RSTピンを無効（I/Oとして使用可能）に設定します。

# 実装例  
 [AVR_MusicBoxの回路例](https://github.com/hiro-otsuka/AVR_MusicBox/tree/master/circuits) を参照。

# 謝辞
本プログラムの試作段階で参考にさせていただいた、Qt や AVRマイコンに関する情報を公開されている先輩方に感謝致します。

# 変更履歴

* 2017/05/22  ソースコードのコメントや実装を整備して正式にリリース
* 2017/04/24  AVR_MBの機能変更に伴いツールも変更
* 2017/03/12  新規公開

