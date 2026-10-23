# IchigoJam P2350 ビルドイメージ (Waveshare RP2350-PiZero / RP2350B)
# 使い方 (BuildKit必須):
#   docker build -o out .
#   → out/IchigoJam_P2350.uf2
#
# ツールチェーン: gcc-arm-none-eabi 14.2.rel1 (Debian trixie版) — 検証済みローカルビルドと同一バージョン

FROM debian:trixie-slim AS builder

# 依存ライブラリのピン留め (2026-10時点の検証済み最新:
#   pico-sdk 2.3.1 / pico-extras 52fd7a7 / PicoDVI mlorenzati fork 9eca921)
ARG PICO_SDK_REF=079c6f39023649b154152db30f1d781e884879bc
ARG PICO_EXTRAS_REF=52fd7a786ce47c989afeaee67e0c6aebee56ed2d
ARG PICODVI_REF=9eca921d17e2390bbeba2284667ba0040643c184

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      git \
      python3 \
      build-essential \
      cmake \
      gcc-arm-none-eabi \
      libnewlib-arm-none-eabi \
      libstdc++-arm-none-eabi-newlib \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /opt

# 依存ライブラリをピン留めコミットで取得
# (--filter=blob:none: 履歴のみ取得で転送量削減、tag/コミットどちらのrefでも指定可)
RUN git clone --filter=blob:none --no-checkout https://github.com/raspberrypi/pico-sdk.git pico-sdk \
 && git -C pico-sdk checkout ${PICO_SDK_REF} \
 && git -C pico-sdk submodule update --init --depth 1 lib/tinyusb \
 && git clone --filter=blob:none --no-checkout https://github.com/raspberrypi/pico-extras.git pico-extras \
 && git -C pico-extras checkout ${PICO_EXTRAS_REF}

# プロジェクトソース (PicoDVI/・build/等は.dockerignoreで除外)
COPY . IchigoJam-P2350

# PicoDVIはCMakeLists.txtがプロジェクト直下を参照するため、ここでcloneする
RUN git clone --filter=blob:none --no-checkout https://github.com/mlorenzati/PicoDVI.git IchigoJam-P2350/PicoDVI \
 && git -C IchigoJam-P2350/PicoDVI checkout ${PICODVI_REF}

# 必須パッチ適用
#  - TinyUSB: タイマー割り込み内からtuh_task()を呼ぶためbusy loop化 (USBキーボード動作に必須)
#  - PicoDVI: SWITCHコマンドでのDVI停止時のDMA IRQ競合修正
RUN patch -p1 -d IchigoJam-P2350/PicoDVI < IchigoJam-P2350/patches/picodvi_dvi_stop_race_fix.patch \
 && patch -p1 -d pico-sdk/lib/tinyusb < IchigoJam-P2350/patches/tinyusb_osal_task_delay.patch

# ビルド (PICO_BOARD=waveshare_rp2350_pizeroはCMakeLists.txtで設定済み)
RUN cmake -S IchigoJam-P2350 -B IchigoJam-P2350/build \
      -DPICO_SDK_PATH=/opt/pico-sdk \
      -DPICO_EXTRAS_PATH=/opt/pico-extras \
 && cmake --build IchigoJam-P2350/build

# 成果物のみのステージ (docker build -o でこのファイルが出力される)
FROM scratch
COPY --from=builder /opt/IchigoJam-P2350/build/IchigoJam_P2350.uf2 /
