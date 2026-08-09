# ALT60

61キー、HHKBを参考にした60%サイズの手配線メカニカルキーボード。PCBを使わず、3Dプリントしたケースで組み立てる前提で設計している。このリポジトリはALT60自体の設計仕様であると同時に、Pro Microベースの手配線キーボードなら何にでも使い回せる小さなフレームワークでもある。

ALT60は、先行して作った手配線プロトタイプ(「ALTKB」)から生まれた。ALTKBはピン配置を1本ずつ発見的に配線して組み上げたもので、動作はするが後から見ると配線の規則性が読み取れない状態になっていた。ALT60はそれをちゃんとやり直したバージョン — 最初から読みやすい配線設計、それに対応したファームウェア設定、そしてその配線を前提に設計した3Dプリント用ケースを揃えている。

## ここに入っているもの

| | |
|---|---|
| [`docs/wiring_plan.html`](docs/wiring_plan.html) | 13×5マトリクスの配線表全体 — 61キーそれぞれがどのピアに乗るか、なぜそうなるか。 |
| [`docs/wiring_layout.html`](docs/wiring_layout.html) | 同じ配線を実際のキー配置に重ねた図 — はんだ付け中に開いておく用。 |
| [`docs/keymap.html`](docs/keymap.html) | 論理キーマップ: ベースレイヤー、Fnレイヤー、JIS/US記号補正。 |
| [`docs/case_design.html`](docs/case_design.html) | 3Dプリント用ケース仕様 — プレート、側壁、ネジボス、MCU用のくぼみ、打鍵傾斜まで全て寸法込み。 |
| [`firmware/`](firmware/) | ファームウェア本体。`keyboard_fw/board_config.h`がALT60固有の設定、`matrix_scan.h`/`keycode_output.h`は汎用。 |
| [`CLAUDE.md`](CLAUDE.md) | AIエージェントによる対話的ピン発見を使って手配線ボードを立ち上げるための指示書 — 別のボードでも使い回せるように書いてある。 |

## キーボード概要

- **61キー**、15U × 5行のHHKB風レイアウト(Row1=14, Row2=14, Row3=13, Row4=12, Row5=8キー)。
- **スペースキーにスタビライザーなし** — 6.25Uのキーキャップの下に独立したスイッチを2個配置し、マトリクスに結線するのは1個だけ。もう1個は機械的な支持専用のダミースイッチ。
- **フルFnレイヤー**: Fキー、矢印、ナビゲーションクラスタ、メディアキー、独立したグレイブ(`` ` ``、Fn+Z)/チルダ(`~`、Fn+X)。HHKB本家のレイアウトを調べた上で設計。
- **実行時JIS/US記号補正**(`Fn+Menu`、EEPROMに保存) — USキーキャップのままWindowsが日本語(JIS)レイアウトになっている環境向け。
- USB経由の**NKRO**、[HID-Project](https://github.com/NicoHood/HID)の`NKROKeyboard`を使用。

## 配線を一言で言うと

列ピン(`D5`〜`D9`)がそれぞれ物理的な行を1本ずつ駆動する。行ピン(`D0`,`D1`,`D2`,`D3`,`D4`,`D10`,`D14`,`D15`,`D16`,`D18`,`D19`,`D20`,`D21`)が行内の左から位置を選ぶ。例外は2つだけ — Backspaceと`\`(それぞれRow1・Row2の14番目のキーで、行ピンが13本しかないため)、Row5の空いている行ピンスロットを間借りする。詳細と理由は[`docs/wiring_plan.html`](docs/wiring_plan.html)を参照。

## ケース

3Dプリント、2パーツ構成: トッププレート(スイッチ穴のみ)とボトムシェル(側壁一体型、小型Pro Micro+ユニバーサル基板用のくぼみ、壁に一体成形したネジボス — バラのスタンドオフ部品なし)。打鍵傾斜(約7.45°、手前18.0mm/奥33.1mm)はHHKB実機の公開寸法を参考にしている。壁・ボス・穴すべての寸法を含む全体仕様は[`docs/case_design.html`](docs/case_design.html)を参照。

## ファームウェアのビルド

```bash
arduino-cli core install arduino:avr
arduino-cli lib install "HID-Project"
arduino-cli compile --fqbn arduino:avr:leonardo firmware/keyboard_fw
arduino-cli upload -p <COM_PORT> --fqbn arduino:avr:leonardo firmware/keyboard_fw
```

`firmware/keyboard_fw/board_config.h`は`docs/wiring_plan.html`から直接導いたALT60の目標設定であり、**まだ実機で配線を検証していない**。ALT60を実際に組む場合は`docs/wiring_layout.html`通りに配線したあと、まず`firmware/matrix_scanner/matrix_scanner.ino`を書き込んで、各キーが`board_config.h`の想定通りの(行, 列)に来ているか1つずつ確認してから信用すること — 立ち上げ手順の全体は`CLAUDE.md`を参照。

## このフレームワークで別のボードを組む場合

`firmware/keyboard_fw/matrix_scan.h`と`keycode_output.h`は完全に汎用で、ボード固有の情報は全て`board_config.h`から読み込む。`firmware/matrix_scanner/matrix_scanner.ino`が対話的なピン発見ツール。`CLAUDE.md`にはその全工程(実際に時間を食った失敗パターンも含めて)がまとめてあるので、AIエージェントが別の人の手配線ボードでも同じ立ち上げ手順を再現できる。

## ライセンス

BSD 3-Clause — [`LICENSE`](LICENSE)参照。公開前に著作権者名を記入すること。
