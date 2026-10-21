# SAV / JSONセーブと5スロットのステート保存

## 通常のセーブ

保存先は `%APPDATA%\Wiiz\Wiiz\saves\<ROMファイル名の拡張子を変更>.sav`。
例：`wiz1p.nes` → `wiz1p.sav`。Q → Save / startupの「Battery save」で `.sav / .json` を切り替え、初期値は `.sav`。
2秒ごと、作品切替時、アプリメニューからの終了時に保存する。
`.sav` は通常の8KiBバイナリSRAMで、独自ヘッダーを付けない。JSONも同じROM名の `.json` として保存する。
実行中の形式切替では現在のデータを新しい形式へ保存し、切替前のファイルを残す。

起動時は選択中の形式を優先する。存在しない場合は、同じROM名の別形式、旧SHA256名のJSON、旧SHA256名のSAVの順に探す。
旧ファイルからの引継ぎでは原本を残し、次回保存時に新しい名前のファイルを作る。
両形式が存在するときは選択中の形式だけを読み、両方の内容を混合しない。
選択したファイルが不正な場合はゲーム起動を止め、既存ファイルを上書きしない。

SAVまたはJSONを編集するときはアプリを終了する。ゲームを選び直した際に読み込まれる。
ROMのハッシュ・形式バージョン、配列の長さ、整数の範囲は確認する。
**キャラクターデータのチェックサム値はJSONに不要で、検査しない。**
ROMに渡すときにチェックサムと複製領域を自動生成する。ROM内の検査処理を改変する方式ではない。
**SAVも同じ補完処理を使う。** 主レコードの値だけ変更してCRCや複製を更新しなくても、読込時に再構成する。
配置を確認できない改訂では生SRAMとして扱い、未検証のチェックサム補完を適用しない。

SAV使用時の探索記録は `<ROM名>.map.json` に分けて保存する。JSON使用時は通常セーブ内の `exploration` に保存する。
SAVと探索記録を移動・バックアップするときは、この2ファイルを一緒に扱う。

形式は `wiiz-battery`、バージョン1。3作品の既知コード配置と、そのパラメータ変更版に対応する。
`characters` は20枠で、次の値を数値として編集できる。

| 項目 | 内容 |
|---|---|
| slot | 1〜20、配列順と一致させる |
| in_use | 原作の登録フラグのバイト値 |
| name_length / name_codes | 名前の長さと、原作文字コード8個（Unicode文字列ではない） |
| race / class / alignment | 原作の種族・職業・性格番号 |
| strength / intelligence / piety / vitality / agility / luck | 各能力値 |
| level | レベル（16bit） |
| age_raw | 原作の年齢フィールドの生バイト値 |
| identity_reserved_bits | Ⅰだけ。種族等に混在する未解釈ビットを保持 |
| unmapped_bytes | 残りのキャラクターデータ。キーはレコード内オフセットの16進文字列 |

`memory_regions` は所持品店の在庫・設定など、キャラクターレコード以外のSRAMをそのまま保持する。
正確さのため未解釈部分を捨てない。チェックサム・複製領域はJSONと二重管理せず、読込時に再構成する。
先頭の `checksum_validation: false` はこの仕様を示す。値を変更しても検査を有効にはしない。
通常セーブはSRAMの保存で、キャラクターがどこまで更新されるかは原作の保存タイミングに従う。

## ステートセーブ・ロード

| 操作 | 保存 | 読込 |
|---|---|---|
| キーボード | F1〜F5 | Shift＋F1〜F5 |
| Q → State save / load | Slot 1〜5を選び、左でSave、X | Slot 1〜5を選び、右でLoad、X |

保存済みスロットへの保存は上書き。Qメニュー内は一時停止したまま操作でき、Zで戻る。
空スロット・壊れたファイル・別ROMのデータの読込失敗時は、現在のゲームを維持してメッセージを表示する。

保存先は `%APPDATA%\Wiiz\Wiiz\states\<ROMのSHA256>\slot1.json` 〜 `slot5.json`。
形式 `wiiz-state` のJSONには、CPU/PPU/APU/Mapper/RAM、完成画面、音声のフィルター・未処理サンプル、
高速化用の等速音楽コアも含める。binjnesの既存 `emulator_write_state` / `emulator_read_state` を使用する。
ネイティブポインターは保存せず、復帰先の新しいコアでポインターを再接続する。
ゲーム用と音楽用の両方が復元できた時点で現在のコアと入れ替える。
入力とホスト側の未再生音声は読込時にクリアし、キーの押しっぱなしや古い音の混入を防ぐ。

ステートは実行途中を正確に戻す用途で、通常セーブの編集用JSONとは別形式。
同じROM・同じコアの版とABI用。別エミュレーターや将来の互換性のないビルドとの交換形式ではない。
破損した内部状態でコアが実行されないよう、ステートのバイナリ部分には整合性ハッシュを付ける。
これはユーザーが不要と指定した通常セーブのキャラクターチェックサムとは別のもの。
ステート読込はSRAMも当時へ戻すため、その後の通常の自動保存も戻した状態を反映する。
速度設定はアプリ設定として維持する。長押しリピートの実時間タイマーはロード時にリセットする。

## 既存実装の再利用・出典

- [bryful/Wiz](https://github.com/bryful/Wiz/tree/020e0b5eb180595d7ce879d8305ed3792c49455d)、MIT、Copyright (c) 2019 bryful。
  WizData.cs / WizIO.csのレコード配置、ChecksumWiz1FC、ChecksumWiz、CRC16.csをC++接続層へ移植した。
  元のC#ソース・LICENSE・固定リビジョン・ハッシュを `third_party/wiz-save/` に保持する。
  C#実行環境は追加せず、Mapper処理は既存binjnesに任せる。
- WizEdit READMEの「CRC-8」は実装上CRC-16/CCITT（0x1021、初期値FFFF）。実装を根拠にした。
  Ⅱ・Ⅲの複製長も実装の `20 * 0x60` に合わせた。
- Ⅰの加算チェックサムは原作固定bankの$F21C〜$F228に合わせる。
  A=8・CLCから8回ADCを行い、途中のcarryを次の加算へ渡し、最後のcarryは捨てる。
  合計値を末尾で折り返す旧Wiiz実装は、この最後のcarryまで加えてしまい、特にパラメータ変更版で登録を消す原因になった。
  CRCと反転・ニブル交換の複製領域は、この正しい加算チェックサムから作り直す。
- [binji/binjnes](https://github.com/binji/binjnes/tree/e2f5871a28ff189daa82b70be9324a42e2aaf9fd)、MIT。
  既存ステートAPIとMapper 1/4の復元処理を使用する。アプリ側は上流のSシリアライザーに画面・音声バッファの保存を加える。
- [nlohmann/json 3.12.0](https://github.com/nlohmann/json/tree/v3.12.0)、MIT。
  公式の未改変ヘッダーを使用。ライセンスとハッシュを同梱し、JSONパーサーを自作しない。

## 検証

```powershell
python tests/check_native_persistence.py output/native-build/windows-vs/Release/wiiz.exe
python tests/check_native_core.py output/native-build/windows-vs/Release/wiiz.exe
python tests/check_native_input.py output/native-build/windows-vs/Release/wiiz.exe
python tests/check_native_settings.py output/native-build/windows-vs/Release/wiiz.exe
```

3作品でJSONのレベル編集→起動、通常セーブの往復、旧savを保持した移行、5スロットの保存・上書きを検査する。
Ⅰ・Ⅱ・Ⅲを4倍速＋等速音楽で、ステート復帰後180更新の全画素・全音声サンプルを中断しない実行と照合する。
破損・別ROM・スロット範囲外の拒否と現在の画面維持、SDLの5個の保存・読込ショートカット、Qメニューからの保存・読込も検査する。

全戦闘・宝箱・全マップでの復帰、異常終了時のファイル耐性、異なるビルド間、Androidは未検証。

追加の設定検査では通常版・パラメータ変更版の計6本で、ROM名の8KiB SAV、CRC未更新のレベル編集を原作が受理すること、
実行中の形式切替、探索記録の別ファイル、旧SHA256名の移行時の原本保持を確認する。

## 再開時の登録消失の修正（2026-10-20）

旧版のⅠ用チェックサム補完の誤りを修正した。最後の桁上がりがある8byteブロックで1違いになり、
原作のロード検査が登録フラグを消す場合があった。能力値だけの残存を検査していた既存テストも修正し、
SAV/JSONそれぞれの再起動後に全20枠の登録フラグが維持されることを照合する。
8byteすべてFFのブロックのチェック値は原作では07で、誤って08にしないことも確認する。

読込元のファイルは補完・起動の前に `saves/backups/<ROM名>/` へ時刻付きでコピーする。
探索記録の別ファイルがある場合は一緒に退避する。バックアップは読込時に作り、自動保存のたびには増やさない。


## 登録slot順の変更

タイトル画面のQ → Roster orderから2枠の交換・上下への移動ができる。レコード全体を交換し、パーティー参照とチェックサムを更新する。交換前の通常セーブは自動バックアップする。[roster-order.md](roster-order.md)に操作と検証範囲を記録する。
