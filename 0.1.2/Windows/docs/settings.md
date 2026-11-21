# 入力・保存形式・メーカー表示の設定

## 入力

| 操作 | キー |
|---|---|
| A・決定 | X / Return（テンキーEnterも対応） |
| B・キャンセル | Z / Backspace |
| Select | A / Tab |
| Start | S / Space |
| 移動 | カーソルキー |
| リセット | Escape |
| アプリ設定 | Q |

決定・キャンセルの別名はゲーム選択・Qメニュー・呪文マップでも使える。
同じボタンの2キーを同時に押した場合、片方だけ離しても、残りのキーが押されている間はボタンを保持する。

## Qメニュー

用途別の小メニューに整理した。CRTと速度は **Display / speed**、下記の設定は **Save / startup** にある。
5枠のステートは **State save / load**。順序とCRTの詳細は[crt.md](crt.md)。

- **Battery save: .sav / .json**：通常セーブの形式。初期値 `.sav`。
  実行中に切り替えると、その時点のSRAMを選んだ形式で保存する。詳細は save-data.md。
- **Skip manufacturer screen: ON / OFF**：メーカー表示を省略するか。初期値OFF。
  起動時とEscapeによるリセット時に適用する。ゲーム途中の切替は次回の起動・リセットから反映する。

設定はユーザー設定フォルダの `battery-format.txt` と `skip-maker.txt` に保持する。
5枠のステート操作とショートカットは従来通り。

## メーカー表示の省略方法

採用済みbinjnes（MIT）の通常のCPU/PPU/APU/Mapper処理で、起動からタイトルの入力待ちまでを
画面・音声を出さずに進める。自動の決定キー入力、ROM書換え、PCの飛び越し、SRAMの初期化は行わない。
初期化・リセットは原作の処理を実行する。既存のRAM書込コールバックを再利用し、新しいコアや依存関係は追加しない。
ライセンス・出典は既存のthird_party/と配布先licenses/に保持する。

| 作品 | タイトル入力待ちの検出（RAM書込後のPC） |
|---|---|
| Ⅰ | PRG16KiB bank 6、$800Dで$0600へ1を書込 |
| Ⅱ | PRG8KiB bank 4、$88A2で$0603へ1を書込 |
| Ⅲ | PRG8KiB bank 4、$890Cで$0603へ1を書込 |

コード配置を照合済みの3作品と各パラメータ変更版が対象。未知のコード配置では省略を適用しない。

## 検証

```powershell
python tests/check_native_settings.py output/native-build/windows-vs/Release/wiiz.exe
```

6本のROMで通常起動と省略時のタイトル画素一致、リセット後の一致、セーブ編集・形式切替を検査する。
SDLの実キーイベントで4組の別名、同時押しの片方だけ解放、Qからの設定変更、Escape後のタイトル到達を確認する。
Androidと全改造ROMの起動表示は未検証。

## Wiz1 bug Fix

Q → Difficulty / bonuses の `Wiz1 bug Fix: OFF / ON` は、8項目を一括で切り替えます。
初期値OFF。設定変更時にはセーブを保持してゲームをリセットします。
詳細・適用条件・出典は [wiz1-bugfix.md](wiz1-bugfix.md) を参照してください。
