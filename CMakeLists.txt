cmake_minimum_required(VERSION 3.13)

# Waveshare RP2350-PiZero (RP2350B / 16MB flash / DVI on GPIO32-39)
# pico-sdk 2.3.1以降に同梱のボード定義を使用する
# (RP2350B=GPIO48本, PICO_PIO_USE_GPIO_BASEは自動有効)
# ボードデフォルトのUART1=GP4/5・I2C1=GP2/3から、IchigoJam P (RP2040版) と同一のGPIO番号
# (UART0=GP0/1, I2C0=GP4/5) に揃える (下のtarget_compile_definitionsで上書き)
# 注意: pico_sdk_import.cmakeはincludeした時点でボード/プラットフォームを確定させるため、
# このsetはincludeより前に置く必要がある (コマンドラインの-DPICO_BOARD=...での上書きも可能)
set(PICO_BOARD waveshare_rp2350_pizero CACHE STRING "Board type")

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(IchigoJam_P2350)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# DVI映像出力ピン (common_dvi_pin_configs.h)
# TMDS: GPIO36/34/32 (+1が各々のN)、CLK: GPIO38 (+1がN)
set(DVI_DEFAULT_SERIAL_CONFIG "waveshare_rp2350_zero" CACHE STRING
	"Select a default pin configuration from common_dvi_pin_configs.h" FORCE)
include_directories(
	${CMAKE_CURRENT_LIST_DIR}/PicoDVI/software/include
)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/PicoDVI/software/libdvi libdvi)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/PicoDVI/software/libsprite libsprite)

add_executable(IchigoJam_P2350
	src/main.c
	src/ntsc/rp2040_pwm_ntsc_textgraph.c
)

target_include_directories(IchigoJam_P2350 PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	# Make sure TinyUSB can find tusb_config.h
	${CMAKE_CURRENT_LIST_DIR}/src
	${CMAKE_CURRENT_LIST_DIR}/src/ntsc
)

# pico-sdk pico-extras PicoDVIはリポジトリ直下にcloneして使う (README参照)
# PicoDVIには patches/picodvi_dvi_stop_race_fix.patch を、
# pico-sdkのTinyUSBには patches/tinyusb_osal_task_delay.patch を適用すること

# PicoDVI/software/apps/mandelbrot/CMakeLists.txt から引用
target_compile_definitions(IchigoJam_P2350 PRIVATE
	DVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG}
	# IchigoJam P (RP2040版) と同一のGPIO番号に揃える (stdio_uartとi2c_defaultがここに追従、i2c.h/i2ceeprom.hは無修正)
	PICO_DEFAULT_UART=0
	PICO_DEFAULT_UART_TX_PIN=0
	PICO_DEFAULT_UART_RX_PIN=1
	PICO_DEFAULT_I2C=0
	PICO_DEFAULT_I2C_SDA_PIN=4
	PICO_DEFAULT_I2C_SCL_PIN=5
	# core0スタック予約を32KBに拡大 (複雑な式でスタック溢れしないように、IchigoJam_Pのmemmap拡大と同じ目的)
	PICO_STACK_SIZE=0x8000
	# core1 (DVIスキャンライン) のスタックはデフォルトの2KBのまま (PICO_CORE1_STACK_SIZEの省略時はPICO_STACK_SIZEに追随するため明示指定)
	PICO_CORE1_STACK_SIZE=0x800
)

# SCRATCH_Y (core0スタック領域) を64KBに拡大 (IchigoJam_Pのmemmap_ichigojam_p.ldの意図をRP2350のメモリマップで再現)
# RP2350 SRAM 520KB = RAM 452KB + SCRATCH_X 4KB + SCRATCH_Y 64KB
pico_set_linker_script_var(IchigoJam_P2350 RAM_LENGTH 0x71000)
pico_set_linker_script_var(IchigoJam_P2350 SCRATCH_X_ORIGIN 0x20071000)
pico_set_linker_script_var(IchigoJam_P2350 SCRATCH_Y_ORIGIN 0x20072000)
pico_set_linker_script_var(IchigoJam_P2350 SCRATCH_Y_LENGTH 0x10000)

target_link_libraries(IchigoJam_P2350 PUBLIC
	pico_stdlib
	pico_multicore
	pico_util
	libdvi
	libsprite
	tinyusb_host
	tinyusb_board
	hardware_flash
	hardware_watchdog
	hardware_adc
	hardware_pwm
	hardware_dma
	hardware_i2c
	hardware_spi
	hardware_sleep
)

# pico_enable_stdio_usb(IchigoJam_P2350 0)
pico_enable_stdio_uart(IchigoJam_P2350 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(IchigoJam_P2350)
