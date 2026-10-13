# IchigoJam P2350

[English](README.md) | [日本語](README_JA.md) | [Manual (Japanese)](MANUAL_JA.md)

A port of **IchigoJam BASIC 1.6.1 (4K version)** for the **Waveshare RP2350-PiZero** (RP2350B) board.

- Video output: **DVI (mini HDMI)**, 640x480p60 (RP2350 SIO built-in TMDS encoders + PIO serializer)
- Keyboard: connect a USB keyboard directly to the Type-C port (native USB host)
- **SD card**: uses the on-board TF slot (slots 200-299 / `P200.IJB`..., plus **ASCII format** `SAVE n,1` → `Pnnn.BAS` for PC interchange)
- Program storage: 100 internal flash slots (0-99) / external EEPROM (100-131) / SD card (200-299)
- The 40-pin header stays dedicated to IchigoJam I/O (usable simultaneously with SD)

The SD card support is based on [Canon-boy4's GivetakeJam_P](https://github.com/Canon-boy4/ichigojam-firm/tree/rp2350-16130), adapted for the on-board TF slot (dedicated pins).

See the [manual (Japanese)](MANUAL_JA.md) for how to flash the firmware. Prebuilt UF2 files are available on the [Releases](../../releases) page.

## Supported board
- [Waveshare RP2350-PiZero](https://www.waveshare.com/rp2350-pizero.htm) (RP2350B, 16MB flash)

## Quick start
1. Download `IchigoJam_P2350.uf2` from [Releases](../../releases)
2. Hold **BOOT**, press and release **RESET**, release BOOT — the board enumerates as a `RPI-RP2` drive
3. Drag & drop the UF2 file — the board reboots and the boot screen appears on the HDMI monitor
4. Connect a USB keyboard to the Type-C port and start coding in BASIC

## Building (for developers)
See [README_JA.md](README_JA.md#環境構築) (Japanese) for the full dependency/patch instructions.

```sh
git clone https://github.com/tomos2027/IchigoJam-P2350.git
cd IchigoJam-P2350
# Obtain pico-sdk / pico-extras / PicoDVI and apply patches (see README_JA.md)
cmake -B build -DPICO_SDK_PATH=/path/to/pico-sdk -DPICO_EXTRAS_PATH=/path/to/pico-extras
cmake --build build
```

## Specifications
| Item | Value |
|---|---|
| BASIC | IchigoJam BASIC 1.6.1 (4K version, arrays [0..357]) |
| `VER()` | 16130 |
| `VER(1)` (platform) | 9 (RP2350) |
| Clock | 252 MHz (DVI TMDS bit clock, VREG 1.20V) |
| Internal storage | 100 slots at the end of flash (4096 bytes each) |
| SD format | FAT32 (MBR/superfloppy), 8.3 names, root only |

## License
- Copyright 2014-2026 the IchigoJam authors. All rights reserved. MIT license. https://ichigojam.net/license
- 4K extensions (4096-byte program area, `IR.IN`, `ENV.IN`, etc.) based on [GivetakeJam_P](https://github.com/Canon-boy4/ichigojam-firm) (MIT license)
- SD card driver (`src/sd_card.h`) is an adapted version of Canon-boy4's GivetakeJam_P (rp2350-16130, MIT license)
- PicoDVI (mlorenzati fork / originally Wren6991 PicoDVI) follows each repository's license

## Trademark
- IchigoJam is a registered trademark of jig.jp co., ltd.
- Raspberry Pi is a registered trademark of the Raspberry Pi Foundation

Please download after agreeing to the terms of use. A license is required for redistribution to third parties. See https://pcn.club/sp/ijp/
