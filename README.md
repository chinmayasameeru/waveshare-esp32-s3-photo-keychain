
<div align="center">

# 📸 Waveshare ESP32-S3 Photo Keychain

**A compact, standalone photo keychain built on the Waveshare ESP32-S3 2-inch No-Touch display.**

Install once over USB. Then use your phone to send a photo over a private, device-hosted Wi-Fi connection — no cloud, no app, no account.

<br>

[![Build](https://github.com/chinmayasameeru/waveshare-esp32-s3-photo-keychain/actions/workflows/build-and-deploy.yml/badge.svg)](https://github.com/chinmayasameeru/waveshare-esp32-s3-photo-keychain/actions/workflows/build-and-deploy.yml)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](#-hardware)
[![Framework](https://img.shields.io/badge/Framework-Arduino-00979D.svg)](#-build-from-source)
[![Display](https://img.shields.io/badge/Display-ST7789T3-6f42c1.svg)](#-hardware)
[![Flash](https://img.shields.io/badge/Flash-16%20MB-4c8fbe.svg)](#-hardware)
[![PSRAM](https://img.shields.io/badge/PSRAM-8%20MB-7b61a8.svg)](#-hardware)
[![Status](https://img.shields.io/badge/Status-Stable-brightgreen.svg)](#-project-status)

<br>

**[⚡ Open the Photo Keychain Installer](https://chinmayasameeru.github.io/waveshare-esp32-s3-photo-keychain/)**

</div>

---

## Overview

This repository is centered on **Photo Keychain**, the stable primary project for the Waveshare ESP32-S3 2-inch No-Touch display.

The intended experience is:

~~~text
USB installation
      ↓
Press BOOT
      ↓
Join PhotoKeychain Wi-Fi
      ↓
Open 192.168.4.1
      ↓
Choose a photo
      ↓
Save
      ↓
Photo appears on the LCD
~~~

The photo transfer is local: the ESP32 hosts a temporary access point and serves the control page directly from the device. The browser prepares the image locally before upload.

The original static USB photo-display firmware is retained as a **secondary legacy project** in [<code>PhotoDisplay-Legacy/</code>](PhotoDisplay-Legacy/) for reference and reproducibility.

### At a glance

| Area | Specification |
|---|---|
| Primary project | **Photo Keychain** |
| MCU | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB OPI |
| Display | 2.0-inch ST7789T3 IPS |
| Resolution | 240 × 320 |
| Touch | None |
| Graphics | LovyanGFX |
| Phone workflow | Local Wi-Fi AP + HTTP |
| AP SSID | <code>PhotoKeychain</code> |
| AP address | <code>192.168.4.1</code> |
| AP password | <code>photo1234</code> |
| Wi-Fi timeout | 5 minutes of HTTP inactivity |
| Firmware toolchain | Arduino-ESP32 3.3.10 |

---

## 🚀 Quick start

### 1. Install the stable firmware

Use the browser installer from a **desktop Chrome or Edge** browser.

### **[👉 Launch the Photo Keychain Installer](https://chinmayasameeru.github.io/waveshare-esp32-s3-photo-keychain/)**

1. Connect the board with a USB-C data cable.
2. Open the installer.
3. Click **Flash firmware**.
4. Select the ESP32-S3 serial port.
5. Wait for installation to complete.
6. The board reboots into Photo Keychain mode.
7. Disconnect USB for standalone use.

> Web Serial is required for the initial USB installation. iPhone/iPad Safari is used later for the local photo-transfer workflow, not for USB flashing.

### 2. Send a photo from your phone

1. Press the board's **BOOT** button.
2. The LCD shows the temporary Wi-Fi connection details.
3. Join **PhotoKeychain** with password **photo1234**.
4. Open **http://192.168.4.1** in Safari.
5. Tap **Choose Photo**.
6. Choose the image.
7. Tap **Save Photo to Keychain**.
8. The board stores the image and displays it.

The browser converts the selected image to a display-ready **320 × 240 JPEG** before transmission.

### 3. Return to normal operation

The access point automatically shuts down after **5 minutes without HTTP activity**. The stored photo remains on the device.

---

## 🔄 Pipeline

The system has two deliberately separate pipelines.

### Firmware release pipeline

~~~text
Git push to main
      │
      ▼
GitHub Actions
      │
      ├── Install Arduino CLI
      ├── Install Arduino-ESP32 3.3.10
      ├── Install LovyanGFX
      ├── Compile PhotoKeychain
      ├── Verify binary outputs
      └── Assemble GitHub Pages bundle
      │
      ▼
Published browser installer
      │
      ▼
Desktop Chrome / Edge
      │
      ├── Web Serial
      └── USB-C
      │
      ▼
ESP32-S3
~~~

### Photo delivery pipeline

~~~text
Phone browser
      │
      ├── Select local photo
      ├── Fit image to display
      ├── Encode 320 × 240 JPEG
      │
      ▼
Private Wi-Fi AP
PhotoKeychain / 192.168.4.1
      │
      ▼
ESP32 WebServer
      │
      ├── POST /save
      ├── Write /photo.tmp
      └── Rename to /photo.jpg
      │
      ▼
FFat storage
      │
      ▼
PSRAM
      │
      ▼
LovyanGFX JPEG decode
      │
      ▼
ST7789T3 LCD
~~~

### Pipeline principles

- **Local processing:** the selected photo is prepared in the browser.
- **Local transfer:** the photo goes directly to the ESP32 AP.
- **Atomic storage update:** the upload is written to a temporary file before replacing the stored photo.
- **Pinned release toolchain:** CI uses a fixed Arduino-ESP32 version.
- **Generated deployment assets:** <code>docs/</code> is produced by CI rather than hand-maintained.

---

## 🧩 Hardware

This project targets the **Waveshare ESP32-S3 2-inch No-Touch Display Development Board**.

| Component | Specification |
|---|---|
| MCU | ESP32-S3R8 |
| External Flash | 16 MB |
| PSRAM | 8 MB OPI |
| LCD | ST7789T3, 2.0-inch IPS |
| Panel resolution | 240 × 320 |
| Logical landscape canvas | 320 × 240 |
| Touch | None |
| Graphics | LovyanGFX |
| Backlight | GPIO 1 |

### LCD pin map

| Signal | GPIO |
|---|---:|
| LCD SCLK | 39 |
| LCD MOSI | 38 |
| LCD MISO | 40 |
| LCD DC | 42 |
| LCD CS | 45 |
| LCD RST | -1 |
| LCD Backlight | 1 |

### Board compatibility

**Supported:** Waveshare ESP32-S3 2-inch **No-Touch** board.

**Not interchangeable:** Waveshare Touch-LCD-2 variants. Those boards add touch/IMU peripherals and different hardware requirements.

---

## 🧠 Firmware architecture

The firmware is intentionally small, stateful, and predictable.

### Boot path

~~~text
Power on
   │
   ├── Initialize GPIO
   ├── Initialize LCD + LovyanGFX
   ├── Disable Wi-Fi
   ├── Mount FFat
   │
   ├── /photo.jpg exists?
   │      ├── yes → PSRAM → JPEG decode → LCD
   │      └── no  → READY screen
   │
   └── Monitor BOOT
~~~

### Photo update path

~~~text
BOOT pressed
   ↓
Start Wi-Fi AP
   ↓
Start WebServer
   ↓
Phone connects
   ↓
GET /
   ↓
Choose photo
   ↓
POST /save
   ↓
Write /photo.tmp
   ↓
Flush + close
   ↓
Replace /photo.jpg
   ↓
Display new photo
   ↓
5-minute inactivity window
   ↓
Wi-Fi OFF
~~~

### Runtime state

| State | Wi-Fi | Display | Purpose |
|---|---|---|---|
| Normal | OFF | Photo / READY | Standalone mode |
| AP active | ON | Connection UI | Phone setup |
| Uploading | ON | Active | Protected from timeout |
| Post-save | ON | Saved photo | Immediate confirmation |
| Timed out | OFF | Stored photo | Return to normal mode |
| Light sleep | OFF | Powered display | Reduce idle activity |

### Storage model

| Item | Value |
|---|---|
| Filesystem | FFat |
| Stored photo | <code>/photo.jpg</code> |
| Temporary upload | <code>/photo.tmp</code> |
| Maximum upload | ~1.5 MB |
| Decode memory | PSRAM |
| Display target | 320 × 240 |

The temporary upload file prevents an incomplete transfer from replacing a known-good photo.

---

## 📱 User manual

### Initial setup

**Prerequisites**

- Waveshare ESP32-S3 2-inch No-Touch board
- USB-C data cable
- Desktop Chrome or Edge for the initial install
- iPhone or another Wi-Fi-capable phone for photo transfer

**Install**

1. Open the Photo Keychain installer.
2. Connect the board over USB.
3. Start the firmware installation.
4. Select the ESP32-S3 serial port.
5. Allow the browser to access the port.
6. Wait for the firmware to be written.
7. Allow the board to reboot.

### Add or replace a photo

1. Press **BOOT**.
2. Join **PhotoKeychain**.
3. Open **http://192.168.4.1**.
4. Choose a photo.
5. Tap **Save Photo to Keychain**.
6. Wait for the LCD to update.

A new photo does not require another firmware flash.

### Network details

| Setting | Value |
|---|---|
| SSID | <code>PhotoKeychain</code> |
| Password | <code>photo1234</code> |
| Address | <code>192.168.4.1</code> |
| Protocol | HTTP |
| Auto-off | 5 minutes of HTTP inactivity |

### Recovery

If the device behaves unexpectedly:

1. Connect it over USB.
2. Run the stable installer again.
3. Let the installer erase and program the device.
4. Reboot.
5. Press BOOT and repeat the phone setup.

For download-mode problems, use the board's BOOT and RESET/EN controls to place the ESP32-S3 into download mode before retrying.

---

## 🛠️ Build from source

CI uses the same board configuration documented here.

### Toolchain

| Tool | Version |
|---|---|
| Arduino-ESP32 | **3.3.10** |
| LovyanGFX | Installed by CI |
| Target | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB OPI |

### FQBN

~~~text
esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB
~~~

### Arduino CLI

~~~bash
arduino-cli config init   --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.10
arduino-cli lib install LovyanGFX

arduino-cli compile   --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB"   --warnings all   --output-dir build   PhotoKeychain
~~~

---

## 🔍 Troubleshooting

<details>
<summary><b>Photo selection does not work on the phone</b></summary>

Reload <code>http://192.168.4.1</code>, make sure the phone is still connected to <code>PhotoKeychain</code>, and choose the photo again.

The stable workflow uses the phone's native file picker and sends the image directly to the device.

</details>

<details>
<summary><b>The phone cannot open 192.168.4.1</b></summary>

Confirm the phone is connected to **PhotoKeychain** rather than another Wi-Fi network.

</details>

<details>
<summary><b>The board does not appear during USB installation</b></summary>

Use a desktop Chrome or Edge browser over HTTPS and a USB-C data cable. Close other applications that may already own the serial port.

</details>

<details>
<summary><b>The LCD is blank or shows an error</b></summary>

Confirm that the hardware is the **No-Touch** variant. Then re-run the stable firmware installer.

</details>

<details>
<summary><b>Wi-Fi turns off</b></summary>

That is expected after approximately **5 minutes of HTTP inactivity**. Press BOOT to begin another photo session.

</details>

---

## 📁 Repository structure

~~~text
.
├── PhotoKeychain/
│   └── PhotoKeychain.ino             # Primary stable firmware
├── PhotoDisplay-Legacy/
│   ├── PhotoDisplay.ino              # Original static USB photo firmware
│   └── README.md                     # Legacy project notes
├── web/
│   ├── index.html                    # Browser installer source
│   └── manifest.json                 # Web-flashing metadata
├── documentation/
│   ├── PIPELINE.md
│   ├── HARDWARE.md
│   ├── FIRMWARE-ARCHITECTURE.md
│   └── USER-MANUAL.md
├── docs/                             # Generated GitHub Pages release bundle
│   ├── index.html
│   ├── PhotoKeychain.bin
│   ├── bootloader.bin
│   ├── partitions.bin
│   ├── boot_app0.bin
│   └── build.txt
├── .github/workflows/
│   └── build-and-deploy.yml
├── CONTRIBUTING.md
├── LICENSE
└── README.md
~~~

> <code>docs/</code> is generated release output. Edit source under <code>PhotoKeychain/</code>, <code>web/</code>, <code>documentation/</code>, and <code>.github/</code>.

---

## 🤝 Contributing

Contributions are welcome.

Before changing firmware, browser behavior, storage, or hardware mappings:

1. Verify the exact Waveshare board variant.
2. Read [<code>CONTRIBUTING.md</code>](CONTRIBUTING.md).
3. Keep the stable user workflow reproducible.
4. Build with the documented toolchain.
5. Test changes on real hardware when they affect display, Wi-Fi, storage, or USB flashing.
6. Update the relevant documentation and test notes.

For browser changes, preserve local image processing and avoid introducing third-party photo uploads.

---

## 📜 License

This project is released under the **MIT License**.

See [<code>LICENSE</code>](LICENSE) for the complete license text.

Third-party libraries and tools remain under their respective licenses.

---

## 🙏 Credits & references

- **[LovyanGFX](https://github.com/lovyan03/LovyanGFX)** — display driver and graphics
- **[Arduino-ESP32](https://github.com/espressif/arduino-esp32)** — ESP32 Arduino core
- **[Waveshare](https://www.waveshare.com/)** — hardware platform

The documentation and browser-installer presentation were informed by the excellent **[Waveshare ESP32-S3 2inch Capacitive Touch Display — Marble Roller Game](https://github.com/OzInFl/Waveshare-ESP32-S3-2inch-Capacitive-Touch-Display-Marble-Roller-Game)** by **OzInFl**.

That reference project targets the **Touch-LCD-2** hardware. This repository targets the **No-Touch** board and is a separate implementation.

---

## 📌 Project status

**Stable.**

The current release is intentionally focused on:

- photo-only operation;
- local phone transfer;
- persistent on-device storage;
- temporary Wi-Fi access point;
- automatic Wi-Fi shutdown;
- browser-based USB firmware installation.

Crop editing, GIF, video, and other media features are deliberately outside the stable release.

---

<div align="center">

### Made for a tiny screen and a very simple workflow.

**[⚡ Launch the Photo Keychain Installer](https://chinmayasameeru.github.io/waveshare-esp32-s3-photo-keychain/)**

</div>
