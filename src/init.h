#ifndef __INIT_H__
#define __INIT_H__
#include "all_includes.h"

//Waveshare RP2350-PiZero (RP2350B)
// TMDS bit clock 270 MHz (CEA-861 720x480p60、VREG 1.20V)
//720*240ピクセル (2bpp、白=0b11/黒=0b00) の画面として扱う
#define FRAME_MAX_WIDTH 720
#define FRAME_MAX_HEIGHT 240
#define FRAME_BYTES_PER_LINE (FRAME_MAX_WIDTH * 2 / 8)
#define VREG_VSEL VREG_VOLTAGE_1_20
#define SCANLINE_INIT 2

//IchigoJam
#define FONT_SIZE 8
//文字は256x192を横9/4に補正して576x192にする
//(720x480は4:3表示時に画素が横長でないため、9/4補正で640x480時と同じ縦横比になる)
#define TEXT_WIDTH (CHAR_MAX_COLS * FONT_SIZE)
#define TEXT_SCALED_WIDTH (TEXT_WIDTH * 9 / 4)
#define MARGIN_WIDTH ((FRAME_MAX_WIDTH - TEXT_SCALED_WIDTH) / 2)
#define MARGIN_HEIGHT ((FRAME_MAX_HEIGHT - CHAR_MAX_ROWS * FONT_SIZE) / 2)

//IchigoJam Pと同一のUART0ピン (GP0=TX、GP1=RX、40ピンヘッダー27/28番ピン) ※keyboard.hのUART_ID (uart0) と対
#define UART_TX_PIN 0
#define UART_RX_PIN 1

//pico
//2bpp (グレースケール) フレームバッファ。白=0b11、黒=0b00
static uint8_t framebuf[FRAME_BYTES_PER_LINE * FRAME_MAX_HEIGHT] __attribute__((aligned(4)));
static uint8_t text_line[TEXT_WIDTH / 8];//1走査線分の文字データ (1bpp、横9/4補正前)

//4ピクセル(4bit)を9ピクセル(18bit)へ横9/4拡大するための表
//各ピクセルは2bpp (白=0b11、黒=0b00)、幅は3,2,2,2ピクセル
static const uint32_t expand9_2bpp[16] = {
    0x00000, 0x0003f, 0x003c0, 0x003ff, 0x03c00, 0x03c3f, 0x03fc0, 0x03fff,
    0x3c000, 0x3c03f, 0x3c3c0, 0x3c3ff, 0x3fc00, 0x3fc3f, 0x3ffc0, 0x3ffff
};

void core1_main() {
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);
    while (true) {
        uint32_t* scanbuf;
        queue_remove_blocking_u32(&dvi0.q_colour_valid, &scanbuf);
        uint32_t* tmdsbuf;
        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
        tmds_encode_2bpp(scanbuf, tmdsbuf, FRAME_MAX_WIDTH);
        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
        queue_add_blocking_u32(&dvi0.q_colour_free, &scanbuf);
    }
}

//この関数の実行時間が長くなるとUSBキーボードを使う時、不具合が起こる
void core1_scanline_callback() {
    // Discard any scanline pointers passed back
    uint32_t* bufptr;
    while (queue_try_remove_u32(&dvi0.q_colour_free, &bufptr))
        ;
    // // Note first two scanlines are pushed before DVI start
    static uint scanline = SCANLINE_INIT;
    bufptr = (uint32_t*)&framebuf[FRAME_BYTES_PER_LINE * scanline];
    queue_add_blocking_u32(&dvi0.q_colour_valid, &bufptr);
    scanline = (scanline + 1) % FRAME_MAX_HEIGHT;
    _g.linecnt++;
}

//1scanline分vramの内容をframebufに反映する
//framebufには720*240ピクセルの2bpp色情報が入っている(白0b11/黒0b00)
//vramには32*24の文字情報が入っている(mag=1の場合)
//_g.screen_bigは0~3の範囲
void vram_to_framebuf_scanline(uint scanline, bool visible_cursor) {
    int vram_y = ((scanline - MARGIN_HEIGHT) / FONT_SIZE) >> _g.screen_big;
    int mag = 1 << _g.screen_big;//文字の大きさの倍率　最大8倍
    if (0 <= vram_y && vram_y < SCREEN_H) {//scanlineが画面の表示範囲なら処理、そうでなければ黒のままでいいので何もしない
        int font_y = ((scanline - MARGIN_HEIGHT) % (FONT_SIZE << _g.screen_big)) >> _g.screen_big;
        uint32_t acc = 0;
        int nbits = 0;
        uint8_t* dst = text_line;
        uint8* c = &vram[vram_y * SCREEN_W];
        for (int vram_x = 0; vram_x < SCREEN_W; vram_x++) {
            unsigned char char_line = CHAR_PATTERN[*c * FONT_SIZE + font_y];
            if (*c >= 0xE0) {
                char_line = screen_pcg[(*c - 0xE0) * FONT_SIZE + font_y];//POKEで書き換えができる文字はscreen_pcgを参照する
            }
            c++;
            char_line ^= 0xff * _g.screen_invert;//VIDEOコマンドでの画面の反転を反映する
            if ((frames >> 4) & visible_cursor && _g.cursorx == vram_x && _g.cursory == vram_y) {//カーソルの位置の文字だけ反転させる
                char_line ^= key_flg.insert ? 0xff : 0xf0;//上書きモードなら文字全体を反転、挿入モードなら文字の左半分を反転
            }
            for (int x = 0; x < FONT_SIZE; x++) {
                uint32_t pixel = (char_line >> (7 - x)) & 0x01;
                for (int k = 0; k < mag; k++) {//文字の拡大 (各ピクセルをmag個複製)
                    acc |= pixel << nbits;
                    if (++nbits == 8) {
                        *dst++ = (uint8_t)acc;
                        acc = 0;
                        nbits = 0;
                    }
                }
            }
        }
        //256ピクセル(1bpp)を576ピクセル(2bpp)へ横9/4拡大する
        //(4ピクセルを9ピクセルへ、幅3,2,2,2で拡大する)
        uint8_t* out = &framebuf[scanline * FRAME_BYTES_PER_LINE + MARGIN_WIDTH * 2 / 8];
        acc = 0;
        nbits = 0;
        for (int i = 0; i < sizeof(text_line); i++) {
            uint8_t b = text_line[i];
            for (int half = 0; half < 2; half++) {
                uint32_t pat = expand9_2bpp[(b >> (half * 4)) & 0x0f];
                acc |= pat << nbits;
                nbits += 18;
                while (nbits >= 8) {
                    *out++ = (uint8_t)acc;
                    acc >>= 8;
                    nbits -= 8;
                }
            }
        }
    }
}

void vram_to_framebuf_all(bool visible_cursor) {
    for (int sl = 0; sl < FRAME_MAX_HEIGHT; sl++) {
        vram_to_framebuf_scanline(sl, visible_cursor);
    }
}

//NTSC用フォントRAM (IchigoJamのフォント+PCG+カーソル表示用の0xFF)
//診断用カナリアで前後を挟む (structで物理レイアウトを保証)
struct {
    volatile uint32_t canary1;
    uint8_t fontram[256 * 8];
    volatile uint32_t canary2;
} ntsc_fg;
#define ntsc_fontram ntsc_fg.fontram
static volatile uint32_t ntsc_diag_error = 0;//診断用: 検知したメモリ破壊 (1:Fontp 2:vram 4:videomode 8:フォントRAM境界 16:DMAバッファ境界 32:走査線カウンタ 64:拡大倍率)
static uint32_t ntsc_diag_frames = 0;//診断用: 破壊を検知した時のフレーム数

//NTSCの1走査線ごとに呼ばれる (_g.linecnt++: TICK()、WAIT -n用)
static void ntsc_line_callback() {
    _g.linecnt++;
}

//1フレーム分vramの内容をNTSCのTVRAMに反映する
//VIDEO 3, 4 (screen_big) のときは1文字をmagxmag文字に複製して拡大表示する
void vram_to_tvram_all(bool visible_cursor) {
    //PCG (0xE0-0xFF) をフォントRAMへ反映 (POKE #700-#7FF対応)
    memcpy(ntsc_fontram + 0xE0 * 8, screen_pcg, SIZE_PCG * 8);

    //カーソルは属性ビット (IJ_ATTR_CURSOR) で描画側に伝える
    //フォントスロット0xFFをカーソルに使うとPCG文字 (0xE0-0xFF) と衝突しゴースト文字が出るため、
    //TVRAMには本来の文字コードを書き、描画時にマスクXORする (DVIと同じ方式)
    ntsc_cursor_mask = key_flg.insert ? 0xff : 0xf0;
    int cursor_pos = -1;
    if (((frames >> 4) & 1) && visible_cursor && _g.cursory >= 0 && _g.cursorx < SCREEN_W && _g.cursory < SCREEN_H) {
        cursor_pos = _g.cursory * SCREEN_W + _g.cursorx;
    }

    uint8 attr = _g.screen_invert ? IJ_ATTR_INVERT : 0;
    int mag = 1 << _g.screen_big;
    ntsc_mag = mag;//NTSC描画側に拡大倍率を伝える (複製セルを拡大グリフの断片として描画する)
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            uint8 c = vram[y * SCREEN_W + x];
            uint8 cellattr = attr | (y * SCREEN_W + x == cursor_pos ? IJ_ATTR_CURSOR : 0);
            uint8* tvram_dst = TVRAM + y * mag * IJ_WIDTH + x * mag;
            uint8* attr_dst = TVRAM + IJ_ATTROFFSET + y * mag * IJ_WIDTH + x * mag;
            for (int j = 0; j < mag; j++) {
                for (int k = 0; k < mag; k++) {
                    tvram_dst[j * IJ_WIDTH + k] = c;
                    attr_dst[j * IJ_WIDTH + k] = cellattr;
                }
            }
        }
    }
}

//NTSCビデオ出力を初期化し開始する (クロックはNTSC_CLOCK_KHZ設定済みのこと)
void ntsc_init() {
    ntsc_fg.canary1 = 0xC0DE6001;//診断用カナリア
    ntsc_fg.canary2 = 0xC0DE6002;
    //IchigoJamのフォントをフォントRAMへ先にコピーしておく (0xE0-0xFFは毎フレームscreen_pcgから反映される)
    memcpy(ntsc_fontram, CHAR_PATTERN, 256 * 8);
    Fontp = ntsc_fontram;
    rp2040_pwm_ntsc_init(NTSC_PIN);
    ntsc_set_line_callback(ntsc_line_callback);
    //IJTEXTモードへ直接設定する (set_videomodeだとstop/start_compositeが走り同期が乱れるため)
    videomode = VMODE_IJTEXT;
    twidth = IJ_WIDTH;
    twidthy = IJ_ROWS;
    attroffset = IJ_ATTROFFSET;
    ntsc_mag = 1 << _g.screen_big;//VIDEOコマンドの拡大設定を引き継ぐ
    clearscreen();
}

//flashの設定セクタからビデオモードを読み込む ([0]はKBD用、[1]をビデオモード、[2]をTFTの180°回転に使用)
uint8_t get_video_mode_config() {
    const uint8_t* config = get_flash(get_config_offset());
    tft_flip = config[2] != 0;
    uint8_t mode = config[1];
    return mode <= VIDEO_MODE_TFT ? mode : VIDEO_MODE_DVI;
}

//ビデオモードをflashの設定セクタへ保存する
void save_video_mode_config(uint8_t mode) {
    uint32_t offset = get_config_offset();
    memcpy(tflash, get_flash(offset), FLASH_SECTOR_SIZE);
    tflash[1] = mode;
    tflash[2] = tft_flip;//TFTの180°回転設定も併せて保存
    if (video_output_mode == VIDEO_MODE_DVI) {
        //DVIは書込中に映像を維持できないため、停止/再開する (停止状態だったら再開しない)
        bool active = video_active();
        video_off(0);
        int save = save_and_disable_interrupts();
        flash_range_erase(offset, FLASH_SECTOR_SIZE);
        flash_range_program(offset, tflash, FLASH_SECTOR_SIZE);
        if (active) {
            video_on();
        }
        restore_interrupts(save);
    } else {
        //NTSCは映像を止めずに書き込む (DMAチェーンはSRAMだけで動くので、割り込み禁止中も同期信号は保たれ、
        //画面は書込中一時フリーズするだけ)。TFTも同様 (SPIはタイマー割り込み駆動のため書込中は更新が止まるだけ)
        int save = save_and_disable_interrupts();
        flash_range_erase(offset, FLASH_SECTOR_SIZE);
        flash_range_program(offset, tflash, FLASH_SECTOR_SIZE);
        restore_interrupts(save);
        if (video_output_mode == VIDEO_MODE_NTSC) {
            //長い割り込み禁止で走査線カウンタが表示位置からずれたので、位相を合わせ直す
            ntsc_release();
            ntsc_init();
        }
    }
}

//DVI/core1が初期化済みか (NTSC/TFTモードで起動したときはSWITCH 0のときまで初期化されない)
static bool dvi_initialized = false;
void picodvi_init();//後ろで定義

//SWITCHコマンドでDVI(HDMI)/NTSC(コンポジット)/TFT(ILI9341)を切り替える
//NTSCは専用クロック (157.5MHz)、DVI/TFTは同一クロック (270MHz) なのでTFT⇔DVIはクロック変更不要
//NTSCとの切替では両システムの停止/再開と、クロック依存の周辺 (UART/PWM/SPI) の再設定を行う
void video_switch_mode(uint8_t mode) {
    bool active = video_active();
    //現在のモードを停止する (core1はDVIのキュー待ちでアイドルになる)
    int save = save_and_disable_interrupts();
    if (video_output_mode == VIDEO_MODE_NTSC) {
        ntsc_release();
    } else if (video_output_mode == VIDEO_MODE_TFT) {
        tft_stop();
    } else {
        dvi_stop(&dvi0);
    }
    if (mode == VIDEO_MODE_NTSC) {
        set_sys_clock_khz(NTSC_CLOCK_KHZ, true);
    } else {
        set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);
    }
    restore_interrupts(save);//割り込み禁止区間は最小にする (NTSCの走査線更新が遅れるため)
    set_uart_bps();
    io_reconfig_pwm();
    sd_reconfig_spi();//SD使用中ならSPI1のbaudを新しいクロックに合わせ直す
    video_output_mode = mode;
    if (mode == VIDEO_MODE_NTSC) {
        ntsc_init();
    } else if (mode == VIDEO_MODE_TFT) {
        tft_start();
    } else {
        if (dvi_initialized) {
            dvi_start(&dvi0);//core1の割込み・キュー待ちは生きているので即再開する
        } else {
            picodvi_init();//NTSC/TFTモードで起動したときはDVIもcore1も未初期化なのでここで初期化する
        }
    }
    save_video_mode_config(mode);
    if (!active) {
        video_off(0);//切替前に映像停止状態だったら停止を維持
    }
}

//SWITCHコマンドの実体 (basic.hのcommand_lcdから呼ばれる)
//SWITCH 0:DVI(HDMI)、SWITCH 1:NTSC(コンポジット)、SWITCH 2:TFT(ILI9341)、SWITCH (引数なし): 次のモードへ
//flip: -1=変更なし (省略/F10トグル時。設定は維持される)、0:TFT通常向き、1:TFT180°回転 (0/1指定時はflashに保存)
void IJB_lcd(uint mode, int flip) {
    if (mode > VIDEO_MODE_TFT) {
        mode = VIDEO_MODE_DVI;
    }
    if (flip >= 0 && tft_flip != (flip != 0)) {
        tft_set_flip(flip != 0);
        screen_clear();//180°回転でTFTの描画座標系が反転するため、画面を消去して残留を防ぐ
        if ((uint8)mode == video_output_mode) {
            save_video_mode_config(video_output_mode);//モード切替なしならここで保存 (切替時はvideo_switch_mode内で保存される)
        }
    }
    _g.display_mode = mode;
    if ((uint8)mode == video_output_mode) {
        return;
    }
    video_switch_mode(mode);//TFTモードへの切替時はtft_startで新しいMADCTL (180°設定) が反映される
}

//hid_app.cが複雑になってるので、もっと簡潔に処理できるなら直したい
void putc_long_press_key() {
    int save = save_and_disable_interrupts();//割り込みを止めないとなぜかtime-us-64() - lp.last_key_report_timeがオーバーフローする時がある
    int interval = lp.is_first_putc ? 500000 : 100000;//キーを長押しした時、最初の1回だけ入力の間隔を長くする
    if (time_us_64() - lp.last_key_report_time > interval) {
        lp.last_key_report_time = time_us_64();
        if (lp.last_char != 0) {
            key_pushc(lp.last_char);
            lp.is_first_putc = false;
        }
    }
    restore_interrupts(save);
}

bool timer(repeating_timer_t* rt) {
    frames++;
    watchdog_update();//ウォッチドッグをfeedする (core0が固まったら自動再起動)
    psg_tick();
    set_tone();
    if (video_active()) {
        if (video_output_mode == VIDEO_MODE_NTSC) {
            //NTSC整合性チェック (診断用): メモリ破壊を検知したらエラー番号を画面に表示し、以降の通常画面更新を止める
            uint32_t err = 0;
            if (Fontp != ntsc_fontram) err |= 1;//フォントポインタ破壊
            if (vram != (uint8*)RAM_VRAM) err |= 2;//VRAMポインタ破壊
            if (videomode != VMODE_IJTEXT) err |= 4;//ビデオモード破壊
            if (ntsc_fg.canary1 != 0xC0DE6001 || ntsc_fg.canary2 != 0xC0DE6002) err |= 8;//フォントRAM境界破壊
            if (ntsc_dma_buf.canary1 != 0xD1A6B001 || ntsc_dma_buf.canary2 != 0xD1A6B002) err |= 16;//DMAバッファ境界破壊
            if (ntsc_scanline() >= NUM_LINES) err |= 32;//走査線カウンタ破壊
            if (ntsc_mag != 1 && ntsc_mag != 2 && ntsc_mag != 4 && ntsc_mag != 8) err |= 64;//拡大倍率破壊
            if (err && !ntsc_diag_error) {
                ntsc_diag_error = err;
                ntsc_diag_frames = frames;
            }
            if (ntsc_diag_error) {
                //TVRAMへ直接エラー番号を書く (VRAM経由でないのでVRAMが壊れていても表示される)
                clearscreen();
                uint8* p = TVRAM;
                *p++ = 'E';
                *p++ = '0' + ntsc_diag_error / 10;
                *p++ = '0' + ntsc_diag_error % 10;
                *p++ = ' ';
                *p++ = 'F';
                uint32 f = ntsc_diag_frames % 1000;
                *p++ = '0' + f / 100;
                *p++ = '0' + (f / 10) % 10;
                *p++ = '0' + f % 10;
            } else {
                vram_to_tvram_all(_g.cursorflg);
            }
        } else if (video_output_mode == VIDEO_MODE_TFT) {
            //TFTは差分更新 (SDがSPI1使用中はスキップ)、走査線カウンタは1フレーム240ラインとする
            tft_update(_g.cursorflg);
            _g.linecnt += FRAME_MAX_HEIGHT;
        } else {
            vram_to_framebuf_all(_g.cursorflg);
        }
    }
    //中でsleep_ms()が呼ばれたときに止まるので、本来タイマーの中でtuh_task()を呼び出してはいけないが、なぜかPicoDVIを動かしていると止まらない
    //pico-sdk/lib/tinyusb/src/osal/osal_pico.h　のosal_task_delay()のsleepをwhileループに置き換えるとPicoDVIなしで一応解決する
    tuh_task();
    putc_long_press_key();
    //LEDの診断表示 (NTSCモード): 通常は約1Hzで点滅、メモリ破壊検知時は高速点滅、点滅が止まっていたらcore0が固まっている
    if (video_output_mode == VIDEO_MODE_NTSC) {
        if (ntsc_diag_error) {
            if ((frames & 7) == 0) gpio_xor_mask(1u << PICO_LED_PIN);
        } else if ((frames & 31) == 0) {
            gpio_xor_mask(1u << PICO_LED_PIN);
        }
    }
    return true;
}

//ハードフォルト時にLEDを高速点滅させる (画面が使えなくても状態を通知する、診断用)
//ウォッチドッグを止めないと8秒後に再起動して点滅が消えてしまう
void isr_hardfault(void) {
    watchdog_disable();
    for (;;) {
        gpio_xor_mask(1u << PICO_LED_PIN);
        busy_wait_us(60000);//約8Hz点滅
    }
}

void on_uart_rx() {
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        if (ch == ESC) {
            _g.key_flg_esc = (_g.uartmode_rxd & 2) == 0; // 1.2b41
        }
        if (_g.uartmode_rxd & 1) {
            key_pushc(ch);
        }
    }
}

//0番のプログラムの先頭行の先頭が @ARUN かどうか
bool is_arun() {
    //0番のプログラムデータ
    //行番号0、行番号1、行の文字数、行の内容...とデータが入っているので、index3からチェックする(行番号1*256+行番号0=行番号)
    const uint8_t* flash = get_flash(calc_flash_offset(0));
    uint8_t arun[] = "@ARUN";
    for (int i = 0; arun[i] != '\0'; i++) {
        if (arun[i] != flash[i + 3]) {
            return false;
        }
    }
    return true;
}

void pico_init() {
    static repeating_timer_t out;
    board_init();

    //flashに保存されたビデオモードで起動する (SWITCHで最後に選択したモード)
    video_output_mode = get_video_mode_config();
    _g.display_mode = video_output_mode;

    vreg_set_voltage(VREG_VSEL);
    sleep_ms(10);//電圧が安定してからクロックを上げる (mlorenzatiのhello_dviデモと同じ)
    // Run system at TMDS bit clock (DVI/TFT) or NTSC clock
    if (video_output_mode == VIDEO_MODE_NTSC) {
        set_sys_clock_khz(NTSC_CLOCK_KHZ, true);
    } else {
        set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);
    }

    stdio_uart_init();//クロックを変えてから初期化する
    // uart_set_hw_flow(UART_ID, false, false);//いる？
    // uart_set_fifo_enabled(UART_ID, false);
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
    gpio_pull_up(UART_TX_PIN);
    gpio_pull_up(UART_RX_PIN);

    //動作確認のため、省電力モード以外のときはLEDポート (GP17) を常時点灯させる (外部LED推奨)
    gpio_init(PICO_LED_PIN);
    gpio_set_dir(PICO_LED_PIN, GPIO_OUT);
    gpio_put(PICO_LED_PIN, 1);

    // init host stack on configured roothub port
    tuh_init(BOARD_TUH_RHPORT);
    tuh_task();//消すと起動時にキーボード接続していた時に、認識しない時がある？
    add_repeating_timer_us(-16666, timer, NULL, &out);//60FPS
    //ウォッチドッグ: core0が固まったら自動再起動する (timer()でfeedする)
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);
}

void picodvi_init() {
    dvi0.timing = &DVI_TIMING;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;//CMakeLists.txtでwaveshare_rp2350_zeroを指定
    dvi0.scanline_callback = core1_scanline_callback;
#if PICO_PIO_USE_GPIO_BASE
    //RP2350-PiZeroのDVI (TMDS=GPIO32-39) をPIOから駆動するため、PIOの32本GPIO窓を16に移動する
    //(pio_gpio_initやlibdviはベースを自動設定しないためアプリ側で必須。mlorenzati/Waveshareのデモと同じ)
    pio_set_gpio_base(DVI_DEFAULT_SERIAL_CONFIG.pio, 16);
#endif
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    // Once we've given core 1 the framebuffer, it will just keep on displaying
    // it without any intervention from core 0
    sprite_fill8(framebuf, 0x00, sizeof(framebuf));
    uint32_t* bufptr = (uint32_t*)framebuf;
    for (int i = 0; i < SCANLINE_INIT; i++) {
        queue_add_blocking_u32(&dvi0.q_colour_valid, &bufptr);
        bufptr += FRAME_BYTES_PER_LINE / 4;
    }

    multicore_launch_core1(core1_main);
    dvi_initialized = true;
}

//起動時に、保存されたモードでビデオ出力を初期化する (main()から呼ばれる)
void video_output_init() {
    if (video_output_mode == VIDEO_MODE_NTSC) {
        ntsc_init();
    } else if (video_output_mode == VIDEO_MODE_TFT) {
        tft_start();
    } else {
        picodvi_init();
    }
}

void ichigojam_init() {
    //SCREEN_W,SCREEN_Hはグローバル変数に置換されるので、基本的にSCREEN_W等を介してアクセスする
    SCREEN_W = CHAR_MAX_COLS;
    SCREEN_H = CHAR_MAX_ROWS;
    screen_clear();

    key_clearKey();
    key_flg.caps = true;
    set_keymap(key_getKeyboardID());
    random_init();
    io_init();
    screen_clp();
    sound_init();
    uart_init_IJ();
    i2c0_init();

    int sleepflg = getSleepFlag();//起動時ボタンを押していたらtrue
    sleepflg |= is_arun();//プログラムの先頭が@ARUNならtrue
    sleepflg |= watchdog_enable_caused_reboot();//SLEEPコマンドで再起動していたらtrue
    if (!sleepflg) {
        psg_beep(10, 3);
    }
    // video_waitSync(BOOT_WAIT);
#ifdef IJB_TITLE
    for (int i = 0;; i++) { // 自動起動時は表示だけしない
        char c = IJB_TITLE[i];
        if (!sleepflg)
            put_chr(c);
        video_waitSync(1);
        if (c == '\n')
            break;
    }
#endif
    if (!sleepflg) {
        put_str("OK\n");
    }
    _g.sleepflg = sleepflg;

}

#endif
