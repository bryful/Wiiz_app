# ゲームパッド（Windows / Android）

SDL 3.2.28のGamepad APIを両OSで共用します。接続・再接続は自動認識で、機種切替やキーコンフィグはありません。
文字ではなく物理位置を固定し、ファミコンと同じく右側をゲームA、下側をゲームBにします。

| 操作 | Xbox | Nintendo Switch Pro |
|---|---|---|
| 移動・項目選択 | 十字キー / 左スティック | 十字キー / 左スティック |
| ゲームA・決定（右） | B | A |
| ゲームB・戻る（下） | A | B |
| Select（左） | X | Y |
| Start（上） | Y | X |
| 設定 | 左スティック押し込み | 左スティック押し込み |
| マップ | RB | R |
| ステート1保存 | LT | ZL |
| ステート1読込 | 右スティック押し込み | 右スティック押し込み |

保存・読込は押した瞬間に直接実行します。右スティックの傾き、LB/L、RT/ZR、中央ボタンには割り当てません。
設定メニューの決定・戻る表示はSDLが報告するラベルに合わせます。
複数パッドの入力は1人用の操作へ合流し、メニュー表記は接続中の先頭パッドを使用します。
キーボードとパッドの押下状態は別管理で、片方の解放で他方の押下は解除されません。
切断・フォーカス喪失・メニュー切替時はゲームへの押下状態を解除します。
スティックとLT/ZLは50%で押下、35%以下で解放。選択画面・設定では方向長押しを350ms後から100ms間隔でリピートします。
ゲーム中の方向リピートは既存のROM・接続層に任せます。

## 再利用・採用理由

- 既存binjnesのコントローラーAPIとアプリのゲーム／メニュー／ステート処理へ接続。
  CPU/PPU/APU、Mapper 1/MMC1・4/MMC3の処理は変更しません。
- 採用済みSDL 3.2.28のデバイス認識、Gamepadマッピング、WindowsのXInput/HIDAPI、Androidの既存Java接続層を再利用。
  出典: https://github.com/libsdl-org/SDL/tree/release-3.2.28
  zlibライセンス、著作権表示と再配布条件は既存のSDLライセンスを保持。追加の実行時依存はありません。
- 既存JSNESの`third_party/jsnes/src/browser/gamepad.js`はブラウザー用であり、ネイティブ版には採用しません。
  `reference/`の解析結果はROM側の確認用で、OSのパッド認識処理には使用しません。
- binjnes上流の入力UIを移植するより、採用済みSDLで正規化して既存アプリ操作へつなぐことで依存を増やさず両OSを共通化できます。

## 検証と未検証範囲

2026-10-21: Windows Release / Android arm64 Debug APKのビルド成功。
SDL仮想パッドで全固定割り当て、LT閾値、左スティックのヒステリシス、メニューリピート、無効時の入力抑止、切断時の解放と再接続を検証。
既存SDL入力リプレイ、および対象3作品の720フレームのコア実行・映像音声・RAM保持リセットを確認。
実機Xbox / Switch ProのUSB・Bluetooth接続、Android実機でのパッド操作、実パッドとタッチ／キーボードの同時操作は未検証です。
互換パッドはOS・SDLの認識モードに依存し、全機種の認識を保証しません。

### Android Proコンの4ボタン補正（2026-10-21）

Windowsはユーザー実機で動作確認済み。Androidではユーザー実機でAB・XYの逆転報告があり、
接続端末（API 37、BSP-D3のSwitchモード057e:2009、Bluetooth）のキー配列とSDL実行時マッピングを調査しました。
SDL 3.2.28のAndroid用`a:b0,b:b1,x:b2,y:b3,sdk>=:29`マッピングを使う当該Proコンだけ、
SDL_SetGamepadMappingで`a:b1,b:b0,x:b3,y:b2`へ補正します。他の入力・Windows・Xbox・HIDAPI・旧Android用の別マッピングは変更しません。
SDLのデバイスマッピングAPIを使う接続層の補正で、上流ソースやエミュレーターコアの改変はありません。
既に補正されたマッピングには再適用しません。追加ライセンス・依存関係はありません。

参照した上流のAndroidマッピングはSDL 3.2.28の`src/joystick/SDL_gamepad_db.h`、
Androidキー変換は`src/joystick/android/SDL_sysjoystick.c`。最新上流にも同じマッピングがあることを確認しました。
4ボタンの補正・二重適用防止・対象外デバイスとWindowsの除外を仮想パッドで検証し、APKを接続端末へ更新しました。
実端末での補正適用ログを確認し、ユーザー操作でも正常と確認済みです。

### Windows Proコンの認識経路（2026-10-21）

WindowsのBSP-D3のSwitchモード（057e:2009、Bluetooth）で、SDLが校正応答を拒否して
`Couldn't load stick calibration`となり、オープン失敗とデバイスID再生成を繰り返していました。
実測で要求0x8010/22に対し応答0x8010/24、要求0x603d/18に対し応答0x603d/25を確認。
SDLの既存SPI処理へ、アドレス一致・要求以上の長さ・応答バッファ内に収まることを検査する互換修正を適用しました。
上流報告とRetroArchでの検証を参照: https://github.com/libsdl-org/SDL/issues/16434
パッチ・ライセンス・出典・ハッシュは`third_party/sdl3/`、再現可能な適用処理は`cmake/PatchSDLSwitch.cmake`。
Windowsでのみ適用し、既存Switch HIDAPIの校正・入力デコードを継続利用します。
Xbox経路、Androidの補正、固定割り当て、ROMコア・Mapperは変更しません。

修正後はSwitchモードのままNintendo Switch Pro Controllerとして開き、ユーザーの実操作で
ボタン32イベント・軸37イベント・アプリ操作18件を受信しました。
決定／戻る、Select／Start、方向、マップ、ステート保存の操作変換を確認。
初回の校正修正後にAB/XYの逆転報告があり、上記の2つの校正応答の組合せを持つBSP-D3だけ
SDL既存の`m_bUseButtonLabels` / `RemapButton`で4ボタンを補正しました。
ユーザーに物理位置で右→下→左→上と操作してもらい、ゲームA→ゲームB→Select→Startの順に
実入力が変換されることを確認済みです。Xbox・通常のPro校正応答・Androidには適用しません。
純正Nintendo Switch Proの実機、振動・センサー、対象ROM上での全ショートカットの操作感は未検証です。
境界値テスト（短い応答・別アドレス・切れたデータの拒否）、既存入力テスト、3作品の720フレームコアテストは成功。
`wiiz_gamepad_check.exe --devices`で認識診断、`--monitor 60000`で60秒の実入力診断を実行できます。

### BSP-D3のWindows接続モード

ユーザー確認で機種はBSP-D3と判明。057e:2009という認識情報だけでは純正Proコンを識別できません。
BSP-D3の説明書はWindows向けにPC X-input Bluetooth Modeを用意しています。
USBを外し、電源OFFからRB＋HOMEを約2秒長押しし、Windowsで「Xbox Wireless Controller」としてペアリングします。
アプリ側は既存SDLのXbox入力経路と固定位置の割り当てを使います。追加ドライバーやキーコンフィグは不要です。
Switchモードでの対応は上記の校正応答修正で実入力を確認済みです。PC用モードへの切替は必須ではありません。
Androidで既に確認された動作は変更しません。

PC用Bluetoothモードへの切替後、Xbox One S Controller（045e:02e0）として認識。
診断開始後の実操作で、SDLボタン34イベント・軸46イベント・アプリ操作22件を受信しました。
右／下ボタンの決定／戻る、左／上ボタンのSelect／Start、十字キー・左スティックの方向操作を確認。
本アプリの固定配置はXboxと同じ物理位置になり、BSP-D3の印字ではなく位置に従います。
対象ROM上でのBluetooth操作感の確認はユーザー側で引き続き行います。

出典: BSP-D3 User Manual、PC X-input Bluetooth Mode（中国語4ページ／英語7ページ）。
https://fccid.io/m/f8a97b955d626b0be541006c351f43c3c674b800c28dfff2184fcef2d389a57e.pdf
説明書の参照のみでソース取り込みはありません。

```powershell
cmake --build --preset windows-release --target wiiz_gamepad_check
./output/native-build/windows-vs/Release/wiiz_gamepad_check.exe
python tests/check_native_input.py output/native-build/windows-vs/Release/wiiz.exe
python tests/check_native_core.py output/native-build/windows-vs/Release/wiiz.exe
```
