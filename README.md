# agri-uv-bringup

DFRobot Gravity UV Index Sensor **SEN0636**（240–370nm、UV/UVA/UVB/UVC、UV Index 0–11 +
Risk Level 0–4 + 生値 mV）の **疎通確認用スケッチ**。値を目視しながら配線・設置位置を
決めるための治具で、最終形ではない。本番は Atom PoE ノード（`agri-uv-poe`）に 1:1 移植予定。

M5Unified がボードを自動判別するので、**1 ソース / 2 env** で 2 種類の LCD ホストに対応する。

| env | ホスト | LCD | UART (UART2) |
|-----|--------|-----|--------------|
| `m5basic-uv-lcd` | M5Stack Basic (Gray) | ILI9341 320×240 | Port C **G16 / G17** |
| `atoms3r-uv-lcd` | M5 AtomS3R | GC9107 128×128 | Grove **G1 / G2** |

UART ピンは build flag `UV_UART_RX` / `UV_UART_TX` で env ごとに注入し、`main.cpp` は
`M5.Display.width()` から small / large レイアウトを実行時に選ぶ。

## ハードウェア

- **センサー**: DFRobot Gravity UV Index Sensor **SEN0636**
  - モード切替スイッチを **UART 側**にしておくこと（毎回確認）
- **ホスト**: M5Stack Basic または M5 AtomS3R（どちらか手元にある方）

### 配線（センサー 4-pin）

```
sensor pin1 (D/R = sensor RX) ← MCU TX
sensor pin2 (C/T = sensor TX) → MCU RX
sensor pin3 (5V)              ← 5V
sensor pin4 (GND)            ← GND
```

- **M5Stack Basic**: RX=G16 / TX=G17（Port C）
- **M5 AtomS3R**: RX=G1 / TX=G2（Grove ポート）

## プロトコル

Modbus RTU, **9600 8N1, slave 0x23**

| input reg | 内容 |
|-----------|------|
| `0x06` | UV raw (mV, 0–3300) |
| `0x07` | UV Index (0–11) |
| `0x08` | Risk Level (0–4 = Low / Moderate / High / Very High / Extreme) |

ライブラリ [`DFRobot/DFRobot_UVIndex240370Sensor`](https://github.com/DFRobot/DFRobot_UVIndex240370Sensor)
は `.h/.cpp` に存在しないヘッダ `#include "String.h"` が混入していて ESP32 core で死ぬため、
pre-build フック [`patch_dfrobot.py`](patch_dfrobot.py) が `.pio/libdeps` 内の該当行を剥がす。

## ビルド / 書き込み

```sh
# AtomS3R
pio run -e atoms3r-uv-lcd -t upload

# M5Stack Basic
pio run -e m5basic-uv-lcd -t upload
```

> AtomS3R は USB-serial チップを持たないため、Serial デバッグは USB-CDC 経由
> （`ARDUINO_USB_CDC_ON_BOOT=1`）。board プロファイルは `m5stack-atoms3` を流用し、
> M5Unified が実機（GC9107 128×128）を自動判別する。

## メモ

- 屋内では蛍光灯 / LED が 240–370nm を素通りするため **idx=0 / risk=0 / mv=0〜2** が正常。
  反応確認は窓越しの日光か UV 懐中電灯を当てる。
- `mv` が固定 0 で無反応ならバス断（配線 / モードスイッチ / slave アドレスを疑う）。
