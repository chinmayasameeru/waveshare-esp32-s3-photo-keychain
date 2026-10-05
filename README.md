<div align="center">

# 📸 Waveshare ESP32-S3 Photo Keychain

**A tiny, self-contained photo device for the Waveshare ESP32-S3 2-inch No-Touch display.**

**Install once over USB. Then update the photo from your phone over a private, device-hosted Wi-Fi network.**

No companion app. No cloud backend. No account.

<br>

[![Build](https://github.com/chinmayasameeru/waveshare-photo-display/actions/workflows/build-and-deploy.yml/badge.svg)](https://github.com/chinmayasameeru/waveshare-photo-display/actions/workflows/build-and-deploy.yml)
[![Status](https://img.shields.io/badge/status-stable-brightgreen.svg)](#-project-status)
[![Platform](https://img.shields.io/badge/platform-ESP32--S3-red.svg)](#-hardware)
[![Framework](https://img.shields.io/badge/framework-Arduino-00979D.svg)](#-build-from-source)
[![Display](https://img.shields.io/badge/display-ST7789T3-6f42c1.svg)](#-hardware)
[![Flash](https://img.shields.io/badge/flash-16%20MB-4c8fbe.svg)](#-hardware)
[![PSRAM](https://img.shields.io/badge/PSRAM-8%20MB%20OPI-7b61a8.svg)](#-hardware)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

<br>

### ⚡ [Open the Photo Keychain Web Installer](https://chinmayasameeru.github.io/waveshare-photo-display/)

</div>

---

## Why this project exists

The display is small enough to carry, but the usual embedded workflow is not.

Photo Keychain turns the board into a simple personal photo device:

~~~text
          INITIAL SETUP
Desktop Chrome / Edge
        │
        │ USB
        ▼
   Install firmware
        │
        ▼
      ESP32-S3
        │
        │ BOOT
        ▼
 Temporary Wi-Fi AP
        │
        │ phone
        ▼
   Choose a photo
        │
        ▼
   Save to keychain
        │
        ▼
  Photo on the LCD
~~~

The stable release deliberately stops there. It is **photo-only by design**.

---

## ✨ What you get

| Capability | Stable behavior |
|---|---|
| Firmware installation | Browser-based USB installer |
| Photo selection | Native phone file picker |
| Photo preparation | Browser-side, 320 × 240 JPEG |
| Photo transfer | Direct to the ESP32 over local Wi-Fi |
| Storage | Persistent FFat filesystem |
| Display | ST7789T3, 240 × 320 |
| Logical canvas | 320 × 240 landscape |
| Wi-Fi | Temporary device-hosted AP |
| AP address | `192.168.4.1` |
| AP SSID | `PhotoKeychain` |
| AP password | `photo1234` |
| Automatic Wi-Fi shutdown | 5 minutes without HTTP activity |
| Normal idle sleep | ~30 seconds before light sleep |
| Cloud account | None |
| Companion mobile app | None |

---

# 🚀 Quick start

## 1. Install the firmware

Open the web installer on a **desktop Chrome or Edge** machine:

### 👉 [Launch the installer](https://chinmayasameeru.github.io/waveshare-photo-display/)

Then:

1. Connect the Waveshare board with a USB-C **data** cable.
2. Open the installer.
3. Click **Flash firmware**.
4. Select the ESP32-S3 serial port.
5. Let the installer erase and program the board.
6. Wait for the automatic reboot.
7. Disconnect USB.

> **Important:** Web Serial is a desktop-browser capability. Use Chrome or Edge for the initial USB installation. iPhone Safari is used for photo transfer after the firmware is installed.

## 2. Connect your phone

After reboot:

1. Press the physical **BOOT** button.
2. The LCD enters iPhone mode and shows the network details.
3. Join Wi-Fi network **`PhotoKeychain`**.
4. Enter password **`photo1234`**.
5. Open **http://192.168.4.1** in Safari.

## 3. Send a photo

On the local Photo Keychain page:

1. Tap **Choose Photo**.
2. Select an image.
3. Wait for the local preparation step.
4. Tap **Save Photo to Keychain**.
5. The new photo is displayed on the LCD.

You do **not** need to re-flash the firmware to change the photo.

## 4. Finish

There is no Finish button.

Simply stop using the local page. After approximately **5 minutes without HTTP activity**, the ESP32 shuts down its Wi-Fi access point and returns to normal operation.

---

# 🔐 Privacy & network model

Photo Keychain is intentionally local.

### During USB installation

The desktop browser downloads the firmware bundle and uses Web Serial to write it to the ESP32-S3.

The installer also loads the browser-side flashing library required to communicate with the ESP32. Your selected photo is not uploaded during this installation step.

### During photo transfer

The phone connects directly to the ESP32's own access point:

~~~text
iPhone
   │
   │  Wi-Fi
   ▼
PhotoKeychain AP
192.168.4.1
   │
   ▼
ESP32 WebServer
   │
   ▼
FFat /photo.jpg
~~~

The selected photo is prepared in the browser and sent directly to the device.

There is no project backend that receives the image.

> The AP credentials are currently fixed in firmware for the local device workflow. They are not intended to provide internet access or account-level security.

---

# 🔄 System pipeline

## Firmware release pipeline

~~~text
Source change
    │
    ▼
GitHub main
    │
    ▼
GitHub Actions
    ├── Arduino CLI
    ├── Arduino-ESP32 3.3.10
    ├── LovyanGFX
    ├── Compile PhotoKeychain
    ├── Verify binaries
    └── Build Pages bundle
    │
    ▼
GitHub Pages
    │
    ▼
Desktop Chrome / Edge
    │
    ▼
Web Serial + USB
    │
    ▼
ESP32-S3
~~~

Every push to `main` is intended to produce a reproducible firmware build and refreshed browser-installer assets.

## Photo pipeline

~~~text
Phone image
    │
    ▼
Browser
    ├── Decode image
    ├── Fit to 320 × 240
    └── Encode JPEG
    │
    ▼
HTTP POST /save
    │
    ▼
ESP32 WebServer
    │
    ├── /photo.tmp
    ├── write upload chunks
    ├── flush + close
    └── rename → /photo.jpg
    │
    ▼
FFat
    │
    ▼
PSRAM
    │
    ▼
LovyanGFX JPEG decoder
    │
    ▼
ST7789T3 LCD
~~~

### Reliability boundary

The firmware does not write the incoming upload directly over the active photo.

It first writes:

`/photo.tmp`

and only promotes that file to:

`/photo.jpg`

after the upload has completed and the final size has been validated.

That keeps a partially uploaded image from becoming the active photo.

---

# 🧩 Hardware

## Supported board

**Waveshare ESP32-S3 2-inch No-Touch Display Development Board**

This project is specifically configured for the **No-Touch** variant.

### Core specifications

| Component | Specification |
|---|---|
| MCU | ESP32-S3R8 |
| CPU | ESP32-S3 dual-core architecture |
| External Flash | 16 MB |
| PSRAM | 8 MB OPI |
| LCD controller | ST7789T3 |
| Display | 2.0-inch IPS |
| Physical resolution | 240 × 320 |
| Firmware canvas | 320 × 240 |
| Touch | None |
| Graphics | LovyanGFX |

## LCD GPIO map

| LCD signal | GPIO |
|---|---:|
| SCLK | 39 |
| MOSI | 38 |
| MISO | 40 |
| DC | 42 |
| CS | 45 |
| RST | -1 |
| Backlight | 1 |

## Board compatibility warning

This project is **not** a drop-in configuration for the Waveshare **Touch-LCD-2**.

The touch board adds additional peripherals and uses different hardware assumptions. Always verify the exact board revision before flashing or changing GPIO definitions.

---

# 🧠 Firmware architecture

The primary firmware is:

[`PhotoKeychain/PhotoKeychain.ino`](PhotoKeychain/PhotoKeychain.ino)

It deliberately keeps the runtime surface small.

## Boot state

~~~text
Power on
   │
   ├── Initialize serial
   ├── Configure backlight
   ├── Initialize LCD
   ├── Set landscape rotation
   ├── Force Wi-Fi OFF
   ├── Mount FFat
   │
   ├── /photo.jpg exists?
   │      ├── YES → PSRAM → JPEG decode → LCD
   │      └── NO  → READY screen
   │
   └── Monitor BOOT
~~~

## Photo-session state

~~~text
BOOT pressed
    │
    ▼
Wi-Fi AP starts
    │
    ▼
HTTP server starts
    │
    ▼
Phone connects
    │
    ▼
GET /
    │
    ▼
Choose Photo
    │
    ▼
POST /save
    │
    ▼
Write temporary file
    │
    ▼
Commit /photo.jpg
    │
    ▼
Display new photo
    │
    ▼
5-minute inactivity timer
    │
    ▼
Wi-Fi OFF
~~~

## HTTP interface

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/` | Serve the local phone UI |
| GET | `/status` | Return basic runtime state |
| POST | `/save` | Receive the photo upload |

Unknown routes are redirected back to the local root address.

## Power management

Two timers shape normal behavior:

| Timer | Behavior |
|---|---|
| ~30 seconds | Normal mode may enter light sleep |
| 5 minutes | Active Wi-Fi AP shuts down after HTTP inactivity |

An upload in progress is protected from the inactivity timeout.

The **BOOT button is GPIO0** and acts as both the photo-session trigger and the configured light-sleep wake source.

---

# 💾 Storage & memory

The stable build uses the 16 MB flash configuration:

~~~text
App 3 MB
FFat 9 MB
Remaining regions
   ↓
Boot / partition / reserved flash areas
~~~

The application stores the active image at:

~~~text
/photo.jpg
~~~

Temporary upload:

~~~text
/photo.tmp
~~~

The current firmware accepts approximately **1.5 MB** per photo upload.

When the photo is displayed, its JPEG data is staged in **8 MB OPI PSRAM** before LovyanGFX performs the decode.

---

# 📱 User manual

## First-time setup

**Hardware**

- Waveshare ESP32-S3 2-inch No-Touch board
- USB-C data cable
- iPhone or Wi-Fi-capable phone

**Software**

- Desktop Chrome or Edge for firmware installation
- Safari or another modern phone browser for local photo transfer

## Change the photo

Press **BOOT**, connect to the AP, open the local address, select the new photo, and save it.

The firmware remains unchanged.

## If Wi-Fi disappears

That is normally expected.

The AP is designed to shut down after about five minutes of inactivity.

Press **BOOT** to start a new photo session.

## If the photo does not update

Start with the simple recovery sequence:

1. Confirm the phone is connected to **PhotoKeychain**.
2. Open **http://192.168.4.1**.
3. Reload the page.
4. Select the image again.
5. Save it again.

If the problem persists, reinstall the stable firmware from the desktop installer.

## If USB flashing fails

Check the following:

- use Chrome or Edge;
- use a USB-C data cable;
- close other serial-monitor applications;
- confirm the ESP32-S3 appears as a serial device;
- use BOOT/RESET to enter download mode when required.

---

# 🛠️ Build from source

## Toolchain

| Tool | Version / configuration |
|---|---|
| Arduino-ESP32 | **3.3.10** |
| Graphics library | **LovyanGFX** |
| Board target | ESP32-S3R8 |
| Flash | **16 MB** |
| PSRAM | **8 MB OPI** |
| Partition scheme | **app3M_fat9M_16MB** |
| USB CDC on boot | **Enabled** |

## FQBN

~~~text
esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB
~~~

## Arduino CLI

~~~bash
arduino-cli config init \
  --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.10
arduino-cli lib install LovyanGFX

arduino-cli compile \
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB" \
  --warnings all \
  --output-dir build \
  PhotoKeychain
~~~

The GitHub Actions workflow is the canonical CI build definition.

---

# 🗂️ Repository structure

~~~text
.
├── PhotoKeychain/
│   └── PhotoKeychain.ino
│
├── PhotoDisplay-Legacy/
│   ├── PhotoDisplay.ino
│   └── README.md
│
├── web/
│   ├── index.html
│   └── manifest.json
│
├── documentation/
│   ├── PIPELINE.md
│   ├── HARDWARE.md
│   ├── FIRMWARE-ARCHITECTURE.md
│   ├── USER-MANUAL.md
│   └── README.md
│
├── docs/
│   ├── index.html
│   ├── PhotoKeychain.bin
│   ├── bootloader.bin
│   ├── partitions.bin
│   ├── boot_app0.bin
│   └── build.txt
│
├── .github/
│   └── workflows/
│       └── build-and-deploy.yml
│
├── CONTRIBUTING.md
├── LICENSE
└── README.md
~~~

### Source of truth

Edit:

- `PhotoKeychain/`
- `web/`
- `documentation/`
- `.github/`
- `README.md`

Treat `docs/` as generated GitHub Pages release output.

---

# 🧪 Stable scope

The stable release intentionally focuses on one excellent workflow:

**photo in → photo displayed**

Included:

- browser-based firmware installation;
- local phone-to-device photo transfer;
- persistent photo storage;
- automatic Wi-Fi shutdown;
- low-power idle behavior;
- recovery through USB reinstallation.

Not included in the stable release:

- crop editor;
- drag/pinch editing;
- GIF playback;
- video playback;
- companion app;
- cloud photo storage.

Keeping these outside the stable release prevents experimental media features from destabilizing the core keychain workflow.

---

# 🤝 Contributing

Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) before opening a pull request.

For the technical reference:

- [Pipeline](documentation/PIPELINE.md)
- [Hardware](documentation/HARDWARE.md)
- [Firmware Architecture](documentation/FIRMWARE-ARCHITECTURE.md)
- [User Manual](documentation/USER-MANUAL.md)

When making hardware changes, document the exact Waveshare board revision and GPIO assumptions.

When changing the phone workflow, preserve the local-first model and test with a real phone and board.

---

# 📜 License

Photo Keychain is released under the **MIT License**.

See [`LICENSE`](LICENSE).

Third-party libraries and services remain under their respective licenses.

---

# 🙏 Credits & reference

Built with:

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [Arduino-ESP32](https://github.com/espressif/arduino-esp32)
- [Waveshare ESP32-S3 hardware](https://www.waveshare.com/)

The documentation and browser-installer presentation were informed by the excellent **[Waveshare ESP32-S3 2inch Capacitive Touch Display — Marble Roller Game](https://github.com/OzInFl/Waveshare-ESP32-S3-2inch-Capacitive-Touch-Display-Marble-Roller-Game)** by **OzInFl**.

That project targets the Touch-LCD-2 hardware. Photo Keychain is a separate implementation for the No-Touch board.

---

# 📌 Project status

**Stable / maintained baseline**

The stable product boundary is intentionally clear:

~~~text
Install firmware
      ↓
Press BOOT
      ↓
Connect phone
      ↓
Select photo
      ↓
Save
      ↓
Display
      ↓
Wi-Fi shuts down
      ↓
Photo remains
~~~

The next feature should be evaluated against the reliability of this core workflow rather than added at the expense of it.

---

<div align="center">

## ⭐ One tiny display. One simple workflow.

### [⚡ Launch the Photo Keychain Web Installer](https://chinmayasameeru.github.io/waveshare-photo-display/)

</div>
