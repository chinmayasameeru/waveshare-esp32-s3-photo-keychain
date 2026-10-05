# Release & Data Pipeline

## Purpose

Photo Keychain has two independent data paths:

1. **Firmware release:** source → GitHub Actions → GitHub Pages → USB → ESP32-S3
2. **Photo transfer:** phone → private Wi-Fi AP → ESP32 HTTP server → FFat → LCD

Keeping these paths separate makes the runtime easier to understand and the release process reproducible.

## Firmware release pipeline

```text
Git push / pull request
        ↓
GitHub Actions
        ↓
Arduino CLI + Arduino-ESP32 3.3.10
        ↓
LovyanGFX installation
        ↓
PhotoKeychain compilation
        ↓
Binary verification
        ↓
GitHub Pages bundle
        ↓
Desktop Chrome / Edge
        ↓
Web Serial over USB-C
        ↓
ESP32-S3 firmware
```

### CI responsibilities

The workflow:

- installs the pinned ESP32 Arduino core;
- installs LovyanGFX;
- compiles the primary `PhotoKeychain/` sketch;
- verifies bootloader, partition table, application, and boot metadata;
- assembles the browser installer bundle;
- publishes the generated assets under `docs/`.

The `docs/` directory is deployment output, not the source of truth.

## Photo pipeline

```text
iPhone / phone
    ↓
Choose Photo
    ↓
Browser decodes image
    ↓
Fit to 320 × 240
    ↓
JPEG encode
    ↓
HTTP POST /save
    ↓
ESP32 writes /photo.tmp
    ↓
Flush + close
    ↓
Atomic rename → /photo.jpg
    ↓
Load JPEG into PSRAM
    ↓
LovyanGFX decode
    ↓
ST7789T3 LCD
```

The photo remains on the local device/browser until it is sent to the keychain's private access point.

## Activity and timeout model

HTTP activity resets the five-minute AP inactivity timer. An active upload is protected from timeout while bytes are being written.

The display itself does not count as Wi-Fi activity. After the inactivity window expires, the AP is stopped and Wi-Fi is powered off.

## Reliability boundaries

The design intentionally contains failures at clear boundaries:

- browser conversion failure → no upload;
- upload/write failure → temporary file is discarded;
- rename failure → previous photo remains available;
- invalid stored photo → firmware shows a diagnostic screen;
- USB flashing failure → board remains recoverable through the ESP32-S3 download mode.

## Release philosophy

The stable branch should prefer small, testable changes over feature breadth. Experimental media features such as crop editors, GIF, and video are not part of the stable release pipeline.
