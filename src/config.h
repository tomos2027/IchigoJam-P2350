// created by Taisuke Fukuno, @taisukef, http://fukuno.jig.jp/
// license CC BY-NC-SA 4.0 http://creativecommons.org/licenses/by-nc-sa/4.0/deed.ja
#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "all_includes.h"

#define ICHIGOJAM

//#define IJB_BETA // beta or not

// #define PSG_TRUE_TONE_UNIT (109 * 1)
// #define PSG_TICK_PER_SEC 60

#define PLATFORM_RP2350 9               //### ここに書くべきか、それが問題だ
#define VER_PLATFORM PLATFORM_RP2350

#define IJB_VER_STR 1.6.1

#ifdef IJB_BETA
#define IJB_TITLE "IchigoJam BASIC 1.5b rv jig.jp\n"
#else
#define IJB_TITLE "IchigoJam BASIC " STRING2(IJB_VER_STR) " by jig.jp forked by na-s.jp\n"
#endif

#define ENABLE_I2C_BPS

//Waveshare RP2350-PiZeroはDVI (HDMI) 出力専用
//NTSCコンポジット出力 (KenKen氏ライブラリのRP2350移植) は将来のフェーズで対応予定、SWITCHコマンドは現状効果なし

//4K版: 1プログラム4096バイト化に伴い保存本数を100→25に (1本=フラッシュ1セクタ4096バイト、GivetakeJam_P準拠)
//SD版: RP2350-PiZeroはフラッシュ16MBのため100本に拡大 (Canon-boy4氏 rp2350-16130準拠、保存域はフラッシュ末尾)
#define N_FLASH_STORAGE 100

#define VERSION15

#define EXT_IOT
// #define KEY_INIT_DELAY
#endif
