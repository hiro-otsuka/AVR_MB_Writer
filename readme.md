# AVR_MB_Writer
このプログラムは、[AVR_MusicBox](https://github.com/hiro-otsuka/AVR_MusicBox/) 用のEEPROM・実行モジュールライタGUIです。
AVR_MusicBox とともに公開している [AVR_MB_Writer.ino](https://github.com/hiro-otsuka/AVR_MusicBox/blob/master/tools/AVR_MB_Writer.ino)
に接続して使用することにより、EEPROMへのファイル書き込み・削除およびATTiny85のFuse設定、実行ファイル書き込みを
簡単な操作で実行できます。

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
 選択したSerialポートに接続します

* BANK  
 接続したSerialポートに接続されている AVR_MusicBoxの EEPROMバンク一覧を表示します。
 表示されたバンク一覧を選択することで対象を切り替えることができます。

* Files  
 選択したBANKからファイルの一覧を取得し、リストに表示します。

* \>\>  
 ファイルの一覧で選択されているEEPROMアドレスを、Addr: に入力します。
 リストの項目を選択することで Addr: に自動入力されるので、通常は使用する必要はありません。

* MML File:  
 MML ファイルを選択します。

* MML2BIN  
 MML2BINツールを使って、MMLファイルをバイナリに変換します。
 エラーや実行結果は、ウィンドウ下部のコンソールに表示されます。
 MML2BINの場所は、File -> Settings メニュー から設定できます。
 バイナリへの変換に成功すると、バイナリファイル名が自動的に Local File: に入力されます。

* Edit  
 MMLファイルをエディタで開きます。
 使用するエディタは、File -> Settings メニュー から設定できます。

* Local File:  
 EEPROMに書き込みを行うファイルを選択します。
 標準では、WAVファイルまたはBINファイルが指定可能です。

* Write!(EEPROM)  
 Local File に指定したファイルを EEPROM に書き込みます。

* Delete!  
 ファイル一覧で選択したファイルを削除します。

* Mode  
 EEROM に対する Write または Delete のモードを選択します。
  Simple Write ... 単純な書き込みです。後続ファイルを破壊します。
  Resize Write ... 選択中のファイルを変更します。後続ファイルは保持されます。
  Insert Write ... 選択中の位置にファイルを挿入します。後続ファイルは保持されます。
 なお、Delete操作では、Resize, Insert で動作の違いはありません（後続ファイルは保持されます）。

* ATTiny ELF:  
 ATTiny85 に書き込む ELFファイル（実行モジュール）を選択します。

* ATTiny85 Fuse:  
 ATTiny85 に書き込む Fuse 設定を選択します。
  Initialize ... AVR_MusicBox に必要な標準的な Fuse を設定します。
  RST Enable ... RSTピンを有効（I/Oとして使用不可）に設定します。
  RST Disable ... RSTピンを無効（I/Oとして使用可能）に設定します。

* Write!(ELF)  
 ATTiny85 に ELF ファイルを書き込みます。

# 実装例  
 [AVR_MusicBoxの回路例](https://github.com/hiro-otsuka/AVR_MusicBox/tree/master/circuits) を参照。

# 謝辞
本プログラムの試作段階で参考にさせていただいた、Qt や AVRマイコンに関する情報を公開されている先輩方に感謝致します。

# 変更履歴

* 2017/03/12  新規公開

