# IchigoJam P2350

[English](README.md) | [日本語](README_JA.md) | [マニュアル (Manual)](MANUAL_JA.md)

IchigoJam BASIC 1.6.1 (4K版) の **Waveshare RP2350-PiZero** (RP2350B) 移植です。

- 映像出力: **3モード切替 (`SWITCH`コマンド)** — **DVI (mini HDMI)** 640x480p60 / **NTSCコンポジット** (GP19+75Ω+560pF外部回路) / **TFT液晶 (ILI9341)** (自由ピン5本で接続、SPI)
- キーボード: Type-CポートにUSBキーボードを直接接続 (ネイティブUSBホスト)
- **SDカード**: 基板内蔵TFスロットに対応 (スロット200-299 / `P200.IJB`〜、**アスキー形式 `SAVE n,1` → `Pnnn.BAS`** でPCとの連携可)
  - `.BAS`は**UTF-8**で保存。LOADはUTF-8 (半角カナ/全角カナ→半角カナ/全角英数→半角の自動変換、BOM対応) と旧形式の生バイトを自動判別。**漢字は非対応 (かなで記述)**
- プログラム保存: 内蔵フラッシュ100本 (0-99) / 外部EEPROM (100-131) / SDカード (200-299)
- 40ピンヘッダーはIchigoJamのI/O専用 (SDと同時利用可)

SDカード機能は [Canon-boy4氏 GivetakeJam_P](https://github.com/Canon-boy4/ichigojam-firm/tree/rp2350-16130) の実装を、基板内蔵TFスロット (専用ピン) 向けに改変して収録しています。

使い方の詳細は **[マニュアル](MANUAL_JA.md)** を参照してください。ビルド済みUF2は [Releases](../../releases) からダウンロードできます。

## 対応基板
- [Waveshare RP2350-PiZero](https://www.waveshare.com/rp2350-pizero.htm) (RP2350B、フラッシュ16MB)

## ピン割り当て (40ピンヘッダー、Pi Zero互換位置)

| 機能 | GP | 物理ピン (BCM) | 機能 | GP | 物理ピン (BCM) |
|---|---|---|---|---|---|
| LED (診断表示兼用) | GP17 | 11 (BCM17) | OUT1 (PWM可) | GP8 | 24 (BCM8) |
| BTN | GP18 | 12 (BCM18) | OUT2 (PWM可) | GP9 | 32 (BCM9) |
| IN1 | GP27 | 13 (BCM27) | OUT3 (PWM可) | GP10 | 23 (BCM11) |
| IN2 | GP26 | 37 (BCM26) | OUT4 (PWM可) | GP11 | 19 (BCM10) |
| IN3 | GP6 | 31 (BCM6) | OUT5 (PWM可) | GP22 | 15 (BCM22) |
| IN4 | GP7 | 26 (BCM7) | OUT6 | GP21 | 40 (BCM21) |
| I2C1 SDA/SCL | GP2/3 | 3/5 (BCM2/3) | SOUND | GP20 | 38 (BCM20) |
| **NTSC映像** | **GP19** | **35 (BCM19)** | UART1 TX/RX | GP4/5 | 8/10 (BCM14/15) |
| **TFT SCK** | **GP14** | **7 (BCM14)** | **TFT CS** | **GP13** | **33 (BCM13)** |
| **TFT DC** | **GP16** | **36 (BCM16)** | **TFT RESET** | **GP12** | **21 (BCM12)** |
| **TFT MOSI** | **GP15** | **29 (BCM15)** | | | |

- **NTSCコンポジット出力 (`SWITCH 1`)**: GP19 → 75Ω直列 + 560pF (GND間) → テレビ等の映像入力
- **TFT液晶 ILI9341 (`SWITCH 2`)**: SCK=GP14、MOSI=GP15、CS=GP13、DC=GP16、RESET=GP12、VCC/LEDA=3.3V、GND=GND (SDO未接続可)。未使用の「自由ピン」のみで接続し、IchigoJamのI/Oポートは消費しない。TFTモード中はSDカードとSPI1を共用するため、SD読み書き中のみ画面更新が一時停止する

- WS.LEDはポート1-7 (OUT1-6＋LED)、IR.INはポート1-4 (IN1-4)
- DVI (HDMI) =GP32-39、**SDカード (TFスロット) =GP30/31/40/43**、PIO-USB=GP28/29 — いずれもIchigoJam I/Oと独立 (同時利用可)
- **本基板はADCピンが40ピンヘッダーに出ていません。** `ANA()`はデジタル値 (LOW=0 / HIGH=1023) を返します
- 未割当ヘッダーGPIO (自由にPOKE可能): GP0/1、GP23-25、GP28/29位置の一部 — **GP12-16とGP19はNTSC/TFT用の映像ピン** (DVI/NTSC/TFTで使わないモードの間は自由にPOKE可)

## 環境構築

**CMake** と **GCC (arm-none-eabi)** をインストールしてください。リポジトリ直下に依存ライブラリをcloneします:

```sh
git clone https://github.com/tomos2027/IchigoJam-P2350.git
cd IchigoJam-P2350

# pico-sdk (2.3.1 / コミット 079c6f3)
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk && git checkout 079c6f3 && git submodule update --init --recursive
# TinyUSBパッチ適用 (USBキーボード利用に必須)
cd lib/tinyusb && git apply ../../../patches/tinyusb_osal_task_delay.patch && cd ../../..

# pico-extras (コミット 52fd7a7 で動作確認)
git clone https://github.com/raspberrypi/pico-extras.git

# PicoDVI (mlorenzati fork / コミット 9eca921 で動作確認)
git clone https://github.com/mlorenzati/PicoDVI.git
cd PicoDVI && git checkout 9eca921
# DVI停止時競合修正パッチ適用 (SWITCHコマンド用、推奨)
git apply ../patches/picodvi_dvi_stop_race_fix.patch && cd ..
```

`IchigoJam_BASIC/` (4K版コア) はリポジトリに同梱済みです。

## ビルド

```sh
cmake -B build
cmake --build build
```

`build/IchigoJam_P2350.uf2` が生成されます (RP2350 ARM-S用)。

**注意**: `set(PICO_BOARD waveshare_rp2350_pizero)` は `pico_sdk_import.cmake` より前に置く必要があります (SDK 2.xはimport時にボードを確定させるため)。CMakeLists.txtはこの構成になっています。

## 書き込み方
1. Type-CをPCに接続せず、**BOOTボタンを押しながらRESET** → 離すと`RPI-RP2`ドライブとして認識
2. `IchigoJam_P2350.uf2`をドラッグ＆ドロップ
3. 自動再起動し、HDMIモニタに「IchigoJam BASIC 1.6.1」が表示される
4. 一度書き込むとType-CにUSBキーボードを直接接続して単体動作できる

## スペック
| 項目 | 値 |
|---|---|
| BASIC | IchigoJam BASIC 1.6.1 (4K版、配列[0..357]) |
| `VER()` | 16130 |
| `VER(1)` (プラットフォーム) | 9 (RP2350) |
| クロック | DVI/TFT: 252MHz、NTSC: 157.5MHz (いずれもオーバークロック、VREG 1.20V) |
| 映像出力 | `SWITCH`コマンドで 0:DVI (HDMI) / 1:NTSCコンポジット / 2:TFT (ILI9341) を切替 (フラッシュに保存) |
| 内部保存 | フラッシュ末尾に100本 (1本4096バイト) |
| SD形式 | FAT32 (MBR/スーパーフロッピー)、8.3名、ルートのみ |

## 既知の注意点
- **TFカードはFAT32フォーマット必須** (SDXCはexFATのためFAT32で再フォーマット、32GB以下推奨)
- `.BAS` (アスキー形式) はUTF-8で保存されます。UTF-8では半角カナ1文字が3バイトになるため、かな中心の長いプログラムは4096バイト制限 (エラー38) に届きやすくなります。**漢字は非対応**です (かなで記述してください)
- 252MHzオーバークロックで動作します。フラッシュチップが非Winbond (Puya等) の場合、QEビット未設定に関する報告があります (PSRAM併用時が主因。本ファームはPSRAM非使用のため通常影響なし。万一固まる場合は`flash_config`の`FLASH_QE_SET_1.uf2`で恒久対策、ただし**2回実行は禁止**)
- SLEEP/WAITのディープスリープはpico-extrasのsleepライブラリ (RP2350対応) を使用
- **NTSCモード (`SWITCH 1`) はCPUが157.5MHz駆動のためBASICの実行速度がDVI/TFTモードより遅くなる** (オリジナルのIchigoJamよりは速い)。信号はGP19から出力、75Ω+560pFの回路を介して接続する。NTSCモード中はGP17のLEDが約1Hzで点滅する (高速点滅は内部異常を検知したとき)
- **TFTモード (`SWITCH 2`) はILI9341 (4線SPI) 専用**。SDカードとSPI1を共用するため、SD読み書き中は画面更新が一時停止する。全画面書き換え (CLS等) は約1フレームかかる。`WAIT -n`の走査線粒度は1フレーム (240走査線) 単位になる
- 映像モードはflashに保存され、次回起動時にそのモードで起動する (F10キーで`SWITCH`、引数なしはDVI→NTSC→TFTの順でトグル)

## ロードマップ
- PIO-USBポート (GP28/29) でのキーボード対応 → Type-CをPCデバッグ兼用に解放

## ライセンス / License
- Copyright 2014-2026 the IchigoJam authors. All rights reserved. MIT license. https://ichigojam.net/license
- 4K版拡張 (プログラム領域4096バイト化、拡張配列、`IR.IN`、`ENV.IN`等) は [GivetakeJam_P](https://github.com/Canon-boy4/ichigojam-firm) 準拠 (MIT license)
- SDカードドライバ (`src/sd_card.h`) は [Canon-boy4氏 GivetakeJam_P](https://github.com/Canon-boy4/ichigojam-firm/tree/rp2350-16130) (rp2350-16130ブランチ、MIT license) の実装をRP2350-PiZero向けに改変したもの
- PicoDVI (mlorenzati fork / 元 Wren6991 PicoDVI): 各リポジトリのライセンスに従う
- NTSC出力 (`src/ntsc/`) は KenKen氏 `rp2040_pwm_ntsc_textgraph` による著作物 (営利目的でなければ自由に利用可、利用明記が推奨)

## 商標 / Trademark
- IchigoJamは、株式会社jig.jpの登録商標です (IchigoJam is a registered trademark of jig.jp co., ltd.)
- Raspberry Pi は、Raspberry Pi財団の登録商標です (Raspberry Pi is a registered trademark of the Raspberry Pi Foundation)

## 利用規約に同意の上、ダウンロードしてください。第三者への配布にはライセンスが必要になります。詳しくはこちら。https://pcn.club/sp/ijp/
- 利用規約（IchigoJamロイヤリティフリープログラム利用規約） https://ichigojam.net/ichigojam-license.pdf
