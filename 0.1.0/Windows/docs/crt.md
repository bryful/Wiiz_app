# CRT表示とQメニュー

## 表示

Q → Display / speed → CRT で **OFF / Mild（弱）/ Standard（標準）** を選ぶ。
左右キーまたはX/Returnで変更し、その場で見た目を確認できる。通常起動の初期値はMild。
設定はユーザー設定フォルダの `crt-mode.txt` に0/1/2で保存する。

- OFF：従来通り256×240の完成画面を最近傍拡大する。
- Mild：弱い走査線、横方向の柔らかい輪郭、ごく薄い発光と周辺減光。
- Standard：走査線と発光を強め、画素の境界をより滑らかにする。

画面の湾曲、映像の欠け、時間方向のちらつき、残像は加えない。
CRT処理は完成した原作画面だけに適用し、Qメニューとアプリ側の呪文マップは鮮明に表示する。
ゲームのCPU/PPU/APU、音楽の速度、入力、セーブ・ステートの内容は変えない。
CRTはアプリの表示設定として保持し、ステートを読んでもその時の設定を維持する。

## 既存処理の採用

調査時に参照した既存コア候補には、そのまま組み込めるCRT処理がなかったため、
[Timothy LottesのCRT shader](https://github.com/libretro/glsl-shaders/blob/e808ca09c699db7a843154cf7dc4dfd766d18a8a/crt/shaders/crt-lottes.glsl)を採用。
ファイル先頭にPublic domainと利用・改変許可の記載がある。元ソース・表記・リビジョン・SHA-256を
`third_party/crt-lottes/`に保持し、配布先の`licenses/crt-lottes/`にも含める。
独立したファイルとして公開されているこの処理だけを使用し、他のライセンスのシェーダーを混在させない。

採用した部分はsRGB変換、Gaussian、Horz3/5/7、Scan/Tri、Bloom。
既存のSDL3 Renderer経路ではGLSLを直接共通利用できないため、これらをC++のCPU処理へ移植した。
GPU専用コンパイラーやOpenGLの依存を増やさず、SDL3の既存テクスチャ転送を再利用する。
MapperやROMには依存しない画面処理なので、既存binjnesのMapper 1/4対応をそのまま使う。
独自のエミュレーターコア・音源は追加しない。

出力は768×720。ガンマ変換とフィルター係数は表にし、水平・垂直の計算を分離する。
前回と同じ完成画面は再計算しない。弱・標準の混合率と周辺減光はアプリ側の表示調整。
画面端はクランプし、原作の枠や文字を切り取らない。

## Qメニューの順序

ゲーム中の最初の画面：

1. Resume game：ゲームへ戻る
2. State save / load：5枠の保存・読込
3. Display / speed：CRT表示・速度
4. Save / startup：SAV/JSON・メーカー表示スキップ
5. Game selection：作品選択へ戻る
6. Select ROM folder：ROMフォルダの変更
7. Exit application：終了

各小メニューは先頭にBackを置き、入った直後は最初の操作項目を選択する。
上下で項目、左右で値やSave/Load、X/Returnで実行。Z/Backspaceは一つ前へ戻り、Qはメニューを閉じる。
終了は最後に配置し、ウィンドウを閉じたときはその項目を選択してメニューを開く。
F1〜F5、Shift＋F1〜F5は小メニューの位置に関係なく従来通り使える。

ゲーム選択中は、表示・保存設定・ROMフォルダ・再検索・終了の5項目だけを表示する。

## 検証と限界

```powershell
python tests/check_native_crt.py output/native-build/windows-vs/Release/wiiz.exe
python tests/check_native_persistence.py output/native-build/windows-vs/Release/wiiz.exe
python tests/check_native_settings.py output/native-build/windows-vs/Release/wiiz.exe
```

実ROMの迷宮画面を使い、移植結果を上流の数式の独立評価と照合する（LUTによる誤差は各色2/255以内）。
元のPPU画面が変更されないこと、OFFへ戻すと従来画面へ戻ること、QパネルがCRT設定にかかわらず同一画素であることも検査する。
実SDLイベントで小メニュー・設定変更・戻る・ステート保存読込・終了の導線を確認する。

Windows x64 Releaseのこの環境では、変化した画面を毎回計算する試験で1フレーム約9ms。
これは描画後処理だけの測定値で、端末全体のFPSの保証ではない。同じ画面の再表示はキャッシュを使う。
ソフトウェア描画でも検証しているが、Android実機・全GPU・全ウィンドウ倍率の画質と性能は未検証。
