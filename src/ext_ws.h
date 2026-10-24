//LED(WS2812B)の仕様は https://akizukidenshi.com/goodsaffix/WS2812B_20200225.pdf 参照

#ifndef __EXT_WS_H__
#define __EXT_WS_H__
#include "all_includes.h"

//1クロックの時間は動作モードにより異なる (DVI/TFT:252MHz=約4ns、NTSC:157.5MHz=約6.3ns)
//100nsあたりのNOP数: 252MHz→25個、157.5MHz→16個
#define NOP5_252 __asm("nop");__asm("nop");__asm("nop");__asm("nop");__asm("nop");
#define NOP4_1575 __asm("nop");__asm("nop");__asm("nop");__asm("nop");
#define SLEEP_100NS_DVI NOP5_252 NOP5_252 NOP5_252 NOP5_252 NOP5_252 //25 nop=約100ns (252MHz)
#define SLEEP_100NS_NTSC NOP4_1575 NOP4_1575 NOP4_1575 NOP4_1575 //16 nop=約102ns (157.5MHz)

static uint8 ws_pin;

//1ビット送信 (bit=1: High 600ns + Low 600ns、bit=0: High 300ns + Low 600ns)
void __not_in_flash_func(wsled_send_one_signal_252) (bool bit) { //DVIモード (252MHz)
    gpio_put(ws_pin, true);
    if (bit) {
        SLEEP_100NS_DVI SLEEP_100NS_DVI SLEEP_100NS_DVI
    }
    SLEEP_100NS_DVI SLEEP_100NS_DVI SLEEP_100NS_DVI
    gpio_put(ws_pin, false);
    SLEEP_100NS_DVI SLEEP_100NS_DVI SLEEP_100NS_DVI SLEEP_100NS_DVI SLEEP_100NS_DVI SLEEP_100NS_DVI
}

void __not_in_flash_func(wsled_send_one_signal_1575) (bool bit) { //NTSCモード (157.5MHz)
    gpio_put(ws_pin, true);
    if (bit) {
        SLEEP_100NS_NTSC SLEEP_100NS_NTSC SLEEP_100NS_NTSC
    }
    SLEEP_100NS_NTSC SLEEP_100NS_NTSC SLEEP_100NS_NTSC
    gpio_put(ws_pin, false);
    SLEEP_100NS_NTSC SLEEP_100NS_NTSC SLEEP_100NS_NTSC SLEEP_100NS_NTSC SLEEP_100NS_NTSC SLEEP_100NS_NTSC
}

//WS.LED port,n[,repeat] (4K版: ポート選択式+拡張配列変数対応)
//port: 1-7 (OUT1-6、7=LED/GP2)、n: LED数、repeat: 送信回数
//色データは配列変数[0]〜[n*3-1]から読み込む (1LEDにつき3変数、[102]-[357]の拡張領域に対応し最大119LED)
void __not_in_flash_func(ws_out) (int port, int nled, int repeat) {
    if (port < 1 || port > 7 || nled < 1 || repeat < 1) {
        return;
    }
    if (nled * 3 > IJB_SIZEOF_ARRAY_MAX) { //配列変数の範囲内に収める (最大119LED)
        nled = IJB_SIZEOF_ARRAY_MAX / 3;
    }
    ws_pin = out_pins[port - 1];
    gpio_init(ws_pin);
    gpio_set_dir(ws_pin, GPIO_OUT);
    gpio_put(ws_pin, false);
    sleep_us(300); //リセット信号
    bool is_ntsc = (video_output_mode == VIDEO_MODE_NTSC);
    int save = save_and_disable_interrupts();
    for (int i = 0; i < repeat; i++) {
        for (int j = 0; j < nled * 3; j++) {//1つのLEDにつき、3つの変数が必要
            uint16 v = *basic_getArrayPtr(j); //拡張配列変数対応 (4K版)
            for (int k = 7; k >= 0; k--) {
                bool bit = v & (0x01 << k);
                if (is_ntsc) {
                    wsled_send_one_signal_1575(bit);
                } else {
                    wsled_send_one_signal_252(bit);
                }
            }
        }
    }
    restore_interrupts(save);
}

#endif
