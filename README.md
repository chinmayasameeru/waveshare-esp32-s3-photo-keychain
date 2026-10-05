# Waveshare ESP32-S3 Photo Display

A simple, reliable photo-display project for the Waveshare ESP32-S3 2inch No-Touch Display Development Board.

The current stable workflow is intentionally simple:

**Choose a photo in the browser → connect the ESP32-S3 by USB → flash → the photo appears on the LCD.**

The firmware itself does **not** use Wi-Fi.

## Hardware

| Item | Specification |
|---|---|
| MCU | ESP32-S3R8 |
| External Flash | 16 MB |
| PSRAM | 8 MB OPI |
| Display | 2-inch ST7789T3 |
| Display resolution | 240 × 320 |
| Touch | No Touch |

### LCD pin mapping

| Signal | GPIO |
|---|---:|
| SCLK | 39 |
| MOSI | 38 |
| MISO | 40 |
| DC | 42 |
| CS | 45 |
| RST | -1 |
| Backlight | 1 |

The display is driven with **LovyanGFX**.

## What the project does

The browser prepares the selected image locally as a **320 × 240 JPEG** and places it in the firmware flash image.

The ESP32 then:

1. boots normally;
2. reads the photo from Flash;
3. copies it into PSRAM;
4. decodes it with LovyanGFX;
5. displays it on the ST7789T3.

The image remains stored in Flash after reset or power cycling.

## Browser flashing

Open the GitHub Pages flasher:

**https://chinmayasameeru.github.io/waveshare-photo-display/**

Use a desktop browser with Web Serial support, such as current **Chrome or Edge**, and connect the ESP32-S3 with USB.

### Flashing steps

1. Open the web flasher.
2. Choose the photo you want displayed.
3. Confirm the browser preview.
4. Connect the ESP32-S3 by USB.
5. Click **Flash Photo Display**.
6. Select the ESP32-S3 serial port.
7. Wait for the flashing process to finish.
8. The board resets and shows the selected photo.

The browser converts the selected image to **320 × 240 JPEG** before flashing.

## Photo storage

The current photo container is stored at:

~~~text
0x310000
~~~

The first 8 bytes are:

| Offset | Size | Meaning |
|---:|---:|---|
| 0x00 | 4 bytes | Photo magic |
| 0x04 | 4 bytes | JPEG size |
| 0x08 | ... | JPEG data |

The firmware validates the container size against the detected Flash size and uses PSRAM for JPEG decoding.

## Build configuration

The GitHub Actions build targets:

~~~text
esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB
~~~

The project uses:

- **Arduino-ESP32 3.3.10**
- **LovyanGFX**
- **16 MB Flash**
- **8 MB OPI PSRAM**
- **app3M_fat9M_16MB partition scheme**

GitHub Actions compiles the firmware and publishes the browser flasher assets to the docs/ directory used by GitHub Pages.

## Important USB note

The browser flasher is the **setup and photo-change mechanism** in the current stable version.

There is currently:

- no Wi-Fi photo upload;
- no mobile Safari photo-transfer mode;
- no BLE;
- no cloud service;
- no companion app.

The photo is transferred to the ESP32 as part of the USB flashing operation.

## Troubleshooting

### LCD is blank

Check:

- USB power is stable.
- The correct firmware build is being flashed.
- The board is the **No-Touch** Waveshare ESP32-S3 display board.
- The display wiring matches the GPIO mapping above.

### Browser cannot find the board

Use:

- desktop Chrome or Edge;
- HTTPS;
- a USB data cable rather than a charge-only cable;
- the browser's Web Serial permission dialog to select the ESP32-S3 port.

### Photo is not displayed

The firmware expects:

- a valid photo container at 0x310000;
- a non-zero JPEG size;
- JPEG data that fits in Flash;
- enough PSRAM for the JPEG buffer.

Re-running the web flasher with a normal JPEG is the simplest recovery path.

## Repository layout

~~~text
.
├── PhotoDisplay/
│   └── PhotoDisplay.ino
├── web/
│   └── index.html
├── docs/
│   ├── index.html
│   ├── bootloader.bin
│   ├── partitions.bin
│   ├── boot_app0.bin
│   └── PhotoDisplay.bin
└── .github/
    └── workflows/
        └── build-and-deploy.yml
~~~

## Project goal

This project is designed as a small standalone photo display with a fast browser-based setup flow and no required desktop IDE for normal flashing.

For the current stable release, the simplest workflow is:

**select photo → USB flash → display photo.**
