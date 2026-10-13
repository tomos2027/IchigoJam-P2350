//main.c以外のファイルにもincludeしているのは、IDEに認識させるため
#ifndef __ALL_INCLUDES_H__
#define __ALL_INCLUDES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef IJB_PICO_NO_PRINT
#define printf(...)
#endif

//pico-sdk
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/flash.h"
#include "hardware/irq.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "hardware/i2c.h"
// SDK定義の後、BASIC命令定義の前にSDドライバーを読み込む (SDON/SDOFF、SAVE/LOAD 200-299)
#include "sd_card.h"

//pico-extras
#include "pico/sleep.h"
#include "hardware/rosc.h"

//PicoDVI
#include "dvi.h"
#include "common_dvi_pin_configs.h"
#include "sprite.h"

//NTSCコンポジット出力 (rp2040_pwm_ntsc_textgraph) は将来フェーズで対応予定

#include "tusb.h"
#include "hid.h"

#define DVI_TIMING dvi_timing_640x480p_60hz

// for IchigoJam BASIC
#define NO_MEMCPY
#define uart_putc ichigojam_uart_putc //picoのものと名前衝突しているのを解決
#include "config.h"
#include "IchigoJam_BASIC/ichigojam-stddef.h"
#include "IchigoJam_BASIC/random.h"
#include "IchigoJam_BASIC/lang.h"
#include "IchigoJam_BASIC/vars.h"
#include "IchigoJam_BASIC/ram.h"
#include "IchigoJam_BASIC/screen.h"
#include "IchigoJam_BASIC/error.h"
#include "IchigoJam_BASIC/ichigojam-jp.fnt.h"
#include "IchigoJam_BASIC/psg.h"
#include "IchigoJam_BASIC/basic.h"
//---
#include "i2c.h"
#include "i2ceeprom.h"
#include "display.h"//storage.h/keyboard.hがvideo_active等を使うため、それより先にincludeする
#include "storage.h"
#include "keyboard.h"
#include "hid_app.c"
#include "io.h"
#include "system.h"
#include "sound.h"
#include "ext_ws.h"
#include "mainc_functions.h"
#include "IchigoJam_BASIC/ext_iot.h"
#include "usr.h"
#include "init.h"

#endif
