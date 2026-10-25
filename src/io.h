#ifndef __IO_H__
#define __IO_H__
#include "all_includes.h"

//Waveshare RP2350-PiZero 40ピンヘッダー (Pi Zero互換位置) のピン割り当て
//ヘッダーの物理番号と BCM番号 (基板シルク表示) をコメントに併記
#define LED 2   // 3番ピン (BCM2)   IchigoJam P (RP2040版) と同一のGPIO番号。基板にLEDがないため外部LEDを推奨、診断表示 (電源ON/ハードフォルト点滅) もこのピン
#define BTN 18  // 12番ピン (BCM18)   GNDとの間にボタンを接続 (内部プルアップ)。IchigoJam PのBTNはGP28だが本基板ではPIO-USBポート (GP28/29) が使用中のためGP18
#define IN1 27  // 13番ピン (BCM27)   以下IchigoJam_P (Pico) と同一のGPIO番号
#define IN2 26  // 37番ピン (BCM26)
#define IN3 6   // 31番ピン (BCM6)
#define IN4 7   // 26番ピン (BCM7)
#define OUT1 8  // 24番ピン (BCM8)
#define OUT2 9  // 32番ピン (BCM9)
#define OUT3 10 // 23番ピン (BCM11)
#define OUT4 11 // 19番ピン (BCM10)
#define OUT5 22 // 15番ピン (BCM22)
#define OUT6 21 // 40番ピン (BCM21)

#define IO_PIN_NUM 11

#define PLEN_MAX 2000
#define ANA_THRESHOLD (1024 / 4)

//ラズパイの動作クロック (DVI:270MHz) に関わらず
//PWMのカウントクロックを常に1MHzにする (clkdiv = sysクロック/1MHz)
//IchigoJamのPWMは1周期20msなので50Hz
//https://rikei-tawamure.com/entry/2021/02/08/213335#PWM%E7%94%A8%E3%82%AB%E3%82%A6%E3%83%B3%E3%82%BF PWMの計算方法は左記参照
#define PWM_CLKDIV_MHZ() (clock_get_hz(clk_sys) / (1000.0f * 1000.0f))
#define PWM_WRAP 19999 // 1MHz / 50Hz - 1

void sound_reconfig();//sound.hで定義 (io.hより後ろにincludeされるため前方宣言)

static uint8 in_pins[] = { IN1, IN2, IN3, IN4, OUT1, OUT2, OUT3, OUT4, BTN, OUT5, OUT6 };
static uint8 out_pins[] = { OUT1, OUT2, OUT3, OUT4, OUT5, OUT6, LED, IN1, IN2, IN3, IN4 };

void IJB_pwm(int port, int plen, int len) {
    if (!(1 <= port && port <= 5)) {
        return;
    }

    if (plen < 0) {
        plen = 0;
    } else if (plen > PLEN_MAX) {
        plen = PLEN_MAX;
    }
    uint8 pin = out_pins[port - 1];
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_clkdiv(slice_num, PWM_CLKDIV_MHZ());
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_gpio_level(pin, (PWM_WRAP + 1) * plen / PLEN_MAX);
    pwm_set_enabled(slice_num, true);
}

//クロックを変更したときに、PWM出力中のOUTポートとサウンドの分周比を再設定する
void io_reconfig_pwm() {
    float div = PWM_CLKDIV_MHZ();
    for (int i = 0; i < 6; i++) {//OUT1-6
        pwm_set_clkdiv(pwm_gpio_to_slice_num(out_pins[i]), div);
    }
    sound_reconfig();
}

/*プルの指定
IN プルアップ(デフォルト)　フロートにも変更可能
OUT 指定しない
*/

void io_init() {
    for (int i = 0; i < 4; i++) {
        uint8 pin = in_pins[i];
        gpio_init(pin);
        gpio_pull_up(pin);
    }
    for (int i = 1; i <= 6; i++) {
        IJB_out(i, 0);
    }
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    gpio_init(BTN);
    gpio_pull_up(BTN);
}

//keycodeが0でないキーは全て反応する
int IJB_btn(int n) {
    if (n < 0) {
        int res = 0;
        for (uint8_t i = 0; i < 6; i++) {
            uint8_t keycode = prev_report.keycode[i];
            uint8_t ascii = keycode < 128 ? keycode2ascii[keycode][0] : 0;
            switch (ascii) {
            case LEFT:
                res |= 1 << 0;
                break;
            case RIGHT:
                res |= 1 << 1;
                break;
            case UP:
                res |= 1 << 2;
                break;
            case DOWN:
                res |= 1 << 3;
                break;
            case ' ':
                res |= 1 << 4;
                break;
            case 'x'://keycode2ascii[][0]は小文字を取得する
                res |= 1 << 5;
                break;
            }
        }
        return res;
    } else if (n == 0) {
        //RP2350-PiZeroのADC対応ピン (GPIO40-47) は40ピンヘッダーに出ていないため、BTNはデジタル入力 (プルアップ)
        return !gpio_get(BTN);
    } else {
        for (uint8_t i = 0; i < 6; i++) {
            uint8_t keycode = prev_report.keycode[i];
            if (0 < keycode && keycode < 128) {
                if (n == keycode2ascii[keycode][0] || n == keycode2ascii[keycode][1]) {
                    return true;
                }
            }
        }
        return false;
    }
}

int IJB_in() {
    int res = 0;
    for (int i = 0; i < IO_PIN_NUM; i++) {
        int pin = in_pins[i];
        bool bit = gpio_get(pin);
        res |= bit << i;
    }
    return res;
}

void IJB_out(int port, int st) {
    if (!(0 <= port && port <= IO_PIN_NUM)) {
        return;
    }
    if (port == 0) {
        for (int i = 0; i < IO_PIN_NUM; i++) {
            gpio_put(out_pins[i], st & (1 << i));
        }
    } else {
        uint8 pin = out_pins[port - 1];
        gpio_init(pin);
        if (st >= 0) {//OUT
            gpio_set_dir(pin, GPIO_OUT);
            gpio_put(pin, st);
        } else if (st == -1) {//IN(フロート)
            gpio_disable_pulls(pin);
        } else if (st == -2) {//IN(プルアップ)
            gpio_pull_up(pin);
        }
    }
}

INLINE void IJB_led(int st) {
    IJB_out(7, st != 0);
}

INLINE int IJB_ana(int n) {
    //RP2350-PiZeroはADC対応ピン (GPIO40-47) が40ピンヘッダーに出ていないため、ANAはデジタル値 (0 or 1023) を返す
    if ((0 <= n && n <= 2) || n == 9) {
        if (n == 0) {
            n = 9;
        }
        int pin = in_pins[n - 1];
        return gpio_get(pin) ? 1023 : 0;
    }
    return 0;
}

INLINE void IJB_clo() {
    io_init();
}

void IJB_dac(int port, int val) {
    //何もしない
}

//BASICのINポート番号→GPIO番号 (4K版、IR.INコマンド用、GivetakeJam_P準拠)
static int ir_port_to_gpio(int port) {
    switch (port) {
        case 1: return IN1;
        case 2: return IN2;
        case 3: return IN3;
        case 4: return IN4;
    }
    return -1;
}

#endif
