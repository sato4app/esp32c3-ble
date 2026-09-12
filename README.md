## esp32-1st-check - ESP32-C3からAndroidにBluetooth接続

ESP32-C3からAndroidにBluetooth接続するWebアプリ
テキストを双方向でやり取りする

## ファイル構成

Web側（ブラウザ）とファーム側（ESP32-C3）を1つのリポジトリで管理する。
ファーム側はPlatformIOプロジェクトとして `firmware/` に分離している。

```
index.html                      Web Bluetooth 側（接続・送受信・ログ表示）
esp32-1st-check.code-workspace  VS Code 用（リポジトリと firmware を同時に開く）
firmware/                       ESP32ファーム（PlatformIOプロジェクト）
  platformio.ini                ボード・ビルド設定（プログラムごとに [env:] を追加）
  src/main.cpp                  BLEサーバ（Nordic UART Service 準拠）
  lib/                          複数ファームで共有するコードの置き場
  .gitignore                    .pio などビルド生成物を除外
```

Web側とファーム側でUUIDを一致させている（Nordic UART Service）。

| 用途 | UUID |
|---|---|
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX（Web → ESP32） | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX（ESP32 → Web） | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` |

## 開発・実行

### ファーム側（ESP32-C3）

VS Codeで `esp32-1st-check.code-workspace` を開き、PlatformIOでビルド・書き込みする。
CLIの場合:

```bash
pio run -d firmware                  # ビルド
pio run -d firmware -t upload        # 書き込み
pio device monitor -b 115200         # シリアルモニタ
```

`ARDUINO_USB_CDC_ON_BOOT=1` を指定しているため、USB経由で `Serial` の出力を確認できる。
書き込みに失敗する場合は、BOOTボタンを押しながらRESETを押して書き込みモードに入る。

BLEライブラリは容量を使う（実測でFlash 75.5%）。機能追加で足りなくなった場合は
`platformio.ini` に `board_build.partitions = huge_app.csv` を追加する。

### Web側

Web Bluetooth APIを使うため、HTTPSまたはlocalhostで開く必要がある。

```bash
python -m http.server 8000
# または npx serve .
# ブラウザで http://localhost:8000 を開く
```

Chrome（Android/デスクトップ）で動作する。SafariとFirefoxはWeb Bluetooth非対応。

## ESP32プログラムを追加する場合

PlatformIOプロジェクトは増やさず、`platformio.ini` に `[env:]` を追加する。
`firmware/src/` にプログラム単位のフォルダを作り、`build_src_filter` で切り替える。

```ini
[env]                          ; 全env共通
platform = espressif32
framework = arduino
monitor_speed = 115200

[esp32c3_base]                 ; ESP32-C3 共通（継承元。envではない）
board = esp32-c3-devkitm-1
build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

[env:c3-ble-chat]
extends = esp32c3_base
build_src_filter = -<*> +<ble-chat/>

[env:c3-ble-sensor]
extends = esp32c3_base
build_src_filter = -<*> +<ble-sensor/>
```

ESP32-S3など別のESP32系ボードへ広げる場合も、`board` を変えた継承元を足すだけでよい。

- 共有コードは `firmware/lib/` に置く（`build_src_filter` の対象外なので全envから使える）
- 別プラットフォーム（RP2040など）は `platform` ごと前提が変わるため、別リポジトリに分ける