# Hardware

## Supported board

**Waveshare ESP32-S3 2-inch No-Touch Display Development Board**

The stable Photo Keychain firmware is written for the No-Touch hardware revision.

> The Waveshare Touch-LCD-2 is a different hardware configuration. Do not copy its touch, IMU, reset, or I²C mappings into this project.

## Core hardware

| Component | Specification |
|---|---|
| MCU | ESP32-S3R8 |
| External Flash | 16 MB |
| PSRAM | 8 MB OPI |
| LCD | ST7789T3 IPS |
| Physical LCD | 2.0 inch |
| Physical resolution | 240 × 320 |
| Firmware logical canvas | 320 × 240 |
| Touch | None |

## LCD mapping

| Signal | GPIO |
|---|---:|
| SCLK | 39 |
| MOSI | 38 |
| MISO | 40 |
| DC | 42 |
| CS | 45 |
| RST | -1 |
| Backlight | 1 |

## Board identification

Before flashing, verify:

- the PCB/model matches the Waveshare 2-inch No-Touch board;
- the display is the ST7789T3 240 × 320 panel;
- the board exposes the BOOT control used by the firmware;
- the USB-C connector supports data.

## Electrical and firmware assumptions

The firmware configures the LCD using LovyanGFX with:

- SPI3 host;
- 40 MHz write clock;
- 16 MHz read configuration;
- display inversion enabled;
- the configured GPIO map above;
- rotation set so application content is 320 × 240.

The backlight is controlled through GPIO 1.

## Storage

The stable build uses the ESP32-S3's 16 MB flash with an application + FFat partition scheme. FFat stores the current photo as `/photo.jpg`.

## Compatibility note

Hardware variants may look similar while exposing different peripherals and pin assignments. Always verify the exact board before modifying the LCD configuration.
