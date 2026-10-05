# Waveshare ESP32-S3 Photo Display

![Build](https://github.com/chinmayasameeru/waveshare-photo-display/actions/workflows/build-and-deploy.yml/badge.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Platform](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)
![Framework](https://img.shields.io/badge/Framework-Arduino-00979D.svg)
![Display](https://img.shields.io/badge/Display-ST7789T3-6f42c1.svg)

A lightweight, browser-flashable photo display for the **Waveshare ESP32-S3 2inch No-Touch Display Development Board**.

The current project is deliberately focused on one reliable workflow:

> **Choose a photo in the browser → connect the ESP32-S3 by USB → flash → the photo appears on the LCD.**

The firmware does not require Wi-Fi, BLE, a companion app, or a cloud service.

---

## ✨ Features

- **Browser-based flashing** through USB Web Serial
- Local image preparation in the browser
- Automatic conversion to **320 × 240 JPEG**
- Photo stored directly in external Flash
- JPEG decoded from Flash using **PSRAM + LovyanGFX**
- ST7789T3 display support for the **No-Touch** Waveshare board
- GitHub Actions build and GitHub Pages deployment
- Reproducible Arduino CLI build configuration
- Simple recovery path: select another image and flash again

---

## 🚀 Flash from your browser

The easiest way to use the project is the GitHub Pages flasher:

[**Open the Photo Display Web Flasher →**](https://chinmayasameeru.github.io/waveshare-photo-display/)

You need a desktop browser with Web Serial support, such as current **Chrome or Edge**.

### Flashing procedure

1. Open the web flasher.
2. Select the image you want displayed.
3. Check the browser preview.
4. Connect the board to the computer with a **USB-C data cable**.
5. Click **Flash Photo Display**.
6. Select the ESP32-S3 serial port.
7. Wait for the firmware and photo to finish flashing.
8. The board resets and displays the selected image.

The browser performs the image conversion locally. The original image is not uploaded to a cloud service by this project.

> **Important:** Web Serial is a desktop-browser capability. The flasher is not intended to run from iPhone Safari or other mobile browsers.

---

## 🧩 Hardware

This project targets the **Waveshare ESP32-S3 2inch No-Touch Display Development Board**.

| Component | Specification |
|---|---|
| MCU | ESP32-S3R8 |
| External Flash | 16 MB |
| PSRAM | 8 MB OPI |
| Display | ST7789T3, 2.0-inch IPS |
| Resolution | 240 × 320 |
| Touch | None |
| Graphics library | LovyanGFX |

### LCD pin map

| Signal | GPIO |
|---|---:|
| SCLK | 39 |
| MOSI | 38 |
| MISO | 40 |
| DC | 42 |
| CS | 45 |
| RST | -1 |
| Backlight | 1 |

The display configuration uses **ST7789T3 inversion enabled** and the board-specific SPI wiring above.

> **Board variant warning:** this repository targets the **No-Touch** board. Do not copy touch-board reset or peripheral pin mappings from other Waveshare ESP32-S3 2-inch projects without checking the exact hardware variant.

---

## 🏗️ How it works

The browser prepares the selected image as a 320 × 240 JPEG and writes a small photo container together with the firmware image.

At boot, the ESP32:

1. initializes the ST7789T3 through LovyanGFX;
2. reads the photo header from Flash;
3. validates the stored JPEG size;
4. copies the JPEG into PSRAM;
5. decodes the JPEG with LovyanGFX;
6. renders it to the LCD.

The photo is stored in Flash, so it remains available across resets and power cycles.

### Photo storage

The photo container starts at:

`0x310000`

The current stable container begins with an 8-byte header:

| Offset | Size | Meaning |
|---:|---:|---|
| `0x00` | 4 bytes | Photo magic |
| `0x04` | 4 bytes | JPEG size |
| `0x08` | variable | JPEG data |

The firmware checks that the recorded JPEG fits within the installed Flash capacity before allocating the JPEG buffer in PSRAM.

---

## 🛠️ Build from source

The repository is built with **Arduino CLI** and GitHub Actions.

### Toolchain

- Arduino-ESP32 **3.3.10**
- LovyanGFX
- ESP32-S3R8 target
- 16 MB Flash
- 8 MB OPI PSRAM

### FQBN

```text
esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB
```

### Arduino CLI

Install Arduino CLI, then:

```bash
arduino-cli config init \
  --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.10
arduino-cli lib install LovyanGFX

arduino-cli compile \
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB" \
  --warnings all \
  --output-dir build \
  PhotoDisplay
```

The GitHub Actions workflow performs the same core build, verifies the expected binary outputs, assembles the GitHub Pages flasher assets, and publishes them under `docs/`.

---

## 🔍 Troubleshooting

### The board does not appear in the browser

- Use desktop **Chrome or Edge**.
- Make sure the page is loaded over **HTTPS**.
- Use a USB-C **data** cable.
- Disconnect other serial applications that may already have the port open.
- Reconnect the board and try the Web Serial port picker again.

### The LCD is blank

- Verify that the exact **No-Touch** Waveshare board is being used.
- Verify the firmware build targets ESP32-S3R8 with 16 MB Flash and OPI PSRAM.
- Check that the LCD wiring matches the pin map in this README.
- Re-run the browser flasher with a normal JPEG.

### The photo is not displayed

The current firmware expects:

- a valid photo container at `0x310000`;
- a non-zero JPEG length;
- JPEG data that fits inside the detected Flash;
- enough PSRAM to hold the JPEG during decoding.

A complete re-flash using the web flasher is the simplest recovery procedure.

---

## 📁 Repository layout

```text
.
├── PhotoDisplay/
│   └── PhotoDisplay.ino          # ESP32-S3 firmware
├── web/
│   └── index.html                # Browser photo flasher
├── docs/
│   ├── index.html                # Published GitHub Pages flasher
│   ├── bootloader.bin
│   ├── partitions.bin
│   ├── boot_app0.bin
│   └── PhotoDisplay.bin
├── .github/
│   └── workflows/
│       └── build-and-deploy.yml  # Build + Pages deployment
├── CONTRIBUTING.md
├── LICENSE
└── README.md
```

The `docs/` firmware binaries are generated by CI; source files under `PhotoDisplay/` and `web/` are the primary project sources.

---

## 🤝 Contributing

Contributions are welcome.

Please read **[CONTRIBUTING.md](CONTRIBUTING.md)** before opening an issue or pull request.

Good contributions include:

- bug fixes;
- build and flashing reliability improvements;
- display-rendering improvements;
- browser UI improvements;
- documentation improvements;
- support for additional image formats or safer image handling;
- reproducible build and CI improvements.

For hardware-related changes, please state the exact Waveshare board variant and any GPIO or wiring changes involved.

---

## 📜 License

This project is released under the **MIT License**. See [LICENSE](LICENSE).

Third-party libraries and tools remain under their respective licenses.

---

## 🙏 Credits & references

The project uses **[LovyanGFX](https://github.com/lovyan03/LovyanGFX)** for the display stack.

The repository structure, presentation style, and browser-flashing documentation were informed by the excellent **[Waveshare ESP32-S3 2inch Capacitive Touch Display — Marble Roller Game](https://github.com/OzInFl/Waveshare-ESP32-S3-2inch-Capacitive-Touch-Display-Marble-Roller-Game)** project by OzInFl. That project targets the **Touch-LCD-2** hardware; this repository is adapted specifically for the **No-Touch** display variant and should be treated as a separate project.

---

## 📌 Project status

The current stable release intentionally prioritizes a simple USB workflow:

**select photo → browser prepares JPEG → USB flash → display photo**

Wi-Fi/iPhone photo transfer is not part of the current stable firmware.

---

## ⭐ Project goal

Make the Waveshare ESP32-S3 2-inch No-Touch display useful as a compact standalone photo frame/keychain display without requiring a desktop IDE for normal photo changes.
