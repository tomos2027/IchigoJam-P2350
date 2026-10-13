#ifndef __DISPLAY_H__
#define __DISPLAY_H__
#include "all_includes.h"

#define CHAR_MAX_COLS 32
#define CHAR_MAX_ROWS 24
struct dvi_inst dvi0;

//Waveshare RP2350-PiZeroはDVI (HDMI) 出力専用 (NTSCコンポジットは将来フェーズで対応予定)

void video_on() {
    SCREEN_W = CHAR_MAX_COLS >> _g.screen_big;
    SCREEN_H = CHAR_MAX_ROWS >> _g.screen_big;
    if (dvi0.dvi_started) {
        return;
    }
    dvi_start(&dvi0);
}

//クロックの変更機能はつけない
void video_off(int clkdiv) {
    dvi_stop(&dvi0);
}

static inline int video_active() {
    return dvi0.dvi_started;
}

void video_waitSync(uint num) {
    //DVIモードでは何もしない (IchigoJam_PのDVIモードと同じ挙動)
}

#endif
