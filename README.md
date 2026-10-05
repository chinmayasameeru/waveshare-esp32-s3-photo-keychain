<div align="center">

# 📸 Waveshare ESP32-S3 Photo Display

**A tiny standalone photo frame for the Waveshare ESP32-S3 2-inch No-Touch display.**

Select an image in your browser → connect by USB → flash → **your photo appears on the LCD.**

<br>

[![Build](https://github.com/chinmayasameeru/waveshare-photo-display/actions/workflows/build-and-deploy.yml/badge.svg)](https://github.com/chinmayasameeru/waveshare-photo-display/actions/workflows/build-and-deploy.yml)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](#-hardware)
[![Framework](https://img.shields.io/badge/Framework-Arduino-00979D.svg)](#-build-from-source)
[![Display](https://img.shields.io/badge/Display-ST7789T3-6f42c1.svg)](#-hardware)
[![Flash](https://img.shields.io/badge/Flash-16%20MB-4c8fbe.svg)](#-hardware)
[![PSRAM](https://img.shields.io/badge/PSRAM-8%20MB-7b61a8.svg)](#-hardware)

<br>

**[⚡ Open the Web Flasher](https://chinmayasameeru.github.io/waveshare-photo-display/)**

</div>

---

## ✨ What this is

A focused ESP32-S3 photo display with a deliberately simple user experience:

> **Choose photo → prepare locally in the browser → flash over USB → display**

The stable firmware is intentionally **offline-first**. It does not require Wi-Fi, BLE, a companion app, or cloud storage.

### At a glance

| | |
|---|---|
| 🧠 **MCU** | ESP32-S3R8 |
| 💾 **Storage** | 16 MB external Flash |
| 🧠 **Memory** | 8 MB OPI PSRAM |
| 🖼️ **Display** | 2.0-inch ST7789T3 IPS |
| 📐 **Panel** | 240 × 320 |
| 🎨 **Graphics** | LovyanGFX |
| 🌐 **Setup** | Browser + USB |
| 📱 **Mobile flashing** | Not supported; use desktop Chrome/Edge |

---

## 🚀 Flash it from your browser

No Arduino IDE is required for normal photo flashing.

### **[👉 Start the Web Flasher](https://chinmayasameeru.github.io/waveshare-photo-display/)**

1. Open the flasher in **desktop Chrome or Edge**.
2. Select the image you want on the display.
3. Check the generated **320 × 240** preview.
4. Connect the board with a **USB-C data cable**.
5. Click **⚡ Flash Photo Display**.
6. Select the ESP32-S3 serial port.
7. Wait for the process to complete.
8. The board resets and shows your photo.

The browser performs the image conversion locally. The project does not send the selected image to a cloud service.

> **Web Serial note:** Web Serial is not available in iPhone Safari, iPad Safari, Firefox, or other unsupported mobile browsers. The current flasher is designed for desktop Chrome/Edge.

---

## 🖼️ Image pipeline

The browser turns the selected source image into a display-ready JPEG before flashing.

```text
┌─────────────────┐
│  Your photo     │
│  JPG / PNG / …  │
└────────┬────────┘
         │
         ▼
┌────────────────────────────┐
│ Browser-side image prepare │
│ • crop to fill             │
│ • fit to frame             │
│ • 320 × 240 output         │
│ • JPEG compression         │
└────────┬───────────────────┘
         │ USB
         ▼
┌────────────────────────────┐
│ ESP32-S3 Flash              │
│ • firmware                  │
│ • photo container           │
└────────┬───────────────────┘
         │
         ▼
┌────────────────────────────┐
│ PSRAM + LovyanGFX           │
│ JPEG decode → ST7789T3 LCD  │
└────────────────────────────┘
```

The photo is stored at **`0x310000`** using a compact 8-byte header:

| Offset | Size | Meaning |
|---:|---:|---|
| `0x00` | 4 bytes | Photo magic |
| `0x04` | 4 bytes | JPEG length |
| `0x08` | variable | JPEG payload |

---

## 🧩 Hardware

This repository targets the **Waveshare ESP32-S3 2inch No-Touch Display Development Board**.

| Component | Specification |
|---|---|
| MCU | ESP32-S3R8 |
| External Flash | 16 MB |
| PSRAM | 8 MB OPI |
| LCD | ST7789T3, 2.0-inch IPS |
| Resolution | 240 × 320 |
| Touch | None |
| Graphics | LovyanGFX |

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

> ⚠️ **No-Touch board only.**  
> The Waveshare Touch-LCD-2 variant uses different peripherals/pin assignments in places. Do not copy touch-board reset or peripheral mappings into this project without verifying the exact hardware revision.

---

## 🧠 Firmware architecture

The firmware keeps the runtime deliberately small.

### Boot

```text
Power on
  │
  ├─► Initialize ST7789T3 / LovyanGFX
  │
  ├─► Read photo header from Flash
  │
  ├─► Validate photo size
  │
  ├─► Allocate JPEG buffer in PSRAM
  │
  ├─► Read JPEG from Flash
  │
  └─► Decode + render to LCD
```

The stored photo survives reset and power cycling because the image is kept in external Flash.

### Design priorities

- **Simple:** one sketch, one browser flasher
- **Local:** image preparation happens in the browser
- **Predictable:** fixed display target and storage location
- **Recoverable:** re-flashing with another image restores the display
- **Lightweight:** JPEG data is staged through PSRAM for decoding

---

## 🛠️ Build from source

GitHub Actions builds the project with a fixed Arduino toolchain and publishes the browser flasher to GitHub Pages.

### Toolchain

| Tool | Version |
|---|---|
| Arduino-ESP32 | **3.3.10** |
| Graphics library | **LovyanGFX** |
| Target | **ESP32-S3R8** |
| Flash | **16 MB** |
| PSRAM | **8 MB OPI** |

### FQBN

```text
esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB
```

### Arduino CLI

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

### Continuous integration

Every change to `main` is built by GitHub Actions. The workflow:

1. installs the pinned ESP32 core;
2. installs LovyanGFX;
3. compiles the ESP32-S3 firmware;
4. verifies the expected binary outputs;
5. assembles the browser flasher files;
6. publishes the generated flasher under `docs/`.

---

## 🔍 Troubleshooting

<details>
<summary><b>🖥️ The board does not appear in the browser</b></summary>

Use desktop **Chrome or Edge** over HTTPS.

Also check:

- the USB-C cable supports data;
- no other application already owns the serial port;
- the board is connected directly or through a known-good USB hub;
- the Web Serial permission dialog is allowed to access the board.

</details>

<details>
<summary><b>🖼️ The LCD is blank after flashing</b></summary>

Confirm that:

- the board is the **No-Touch** variant;
- the firmware target is ESP32-S3R8;
- Flash is configured as 16 MB;
- PSRAM is configured as OPI;
- the selected source image was accepted by the browser.

Re-run the web flasher with a normal JPEG as the simplest recovery procedure.

</details>

<details>
<summary><b>📷 The photo is not displayed</b></summary>

The firmware expects a valid photo container at `0x310000` with:

- a valid magic value;
- a non-zero JPEG length;
- a JPEG that fits within installed Flash;
- enough PSRAM for temporary JPEG storage.

A complete re-flash is the recommended recovery path.

</details>

<details>
<summary><b>🔌 USB flashing fails</b></summary>

Use the board's USB-C connection with a data-capable cable and try desktop Chrome/Edge again.

If the board does not enter download mode automatically, use the board's **BOOT** and **RESET/EN** controls to enter the ESP32-S3 download mode, then retry the flasher.

</details>

---

## 📁 Repository layout

```text
.
├── PhotoDisplay/
│   └── PhotoDisplay.ino          # ESP32-S3 firmware
├── web/
│   └── index.html                # Browser photo flasher source
├── docs/                         # GitHub Pages output generated by CI
│   ├── index.html
│   ├── bootloader.bin
│   ├── partitions.bin
│   ├── boot_app0.bin
│   └── PhotoDisplay.bin
├── .github/
│   └── workflows/
│       └── build-and-deploy.yml  # Build + deployment pipeline
├── CONTRIBUTING.md
├── LICENSE
└── README.md
```

> **Generated files:** `docs/` is release output generated by GitHub Actions. Make source changes in `PhotoDisplay/`, `web/`, `.github/workflows/`, and the documentation files.

---

## 🤝 Contributing

Contributions are welcome and encouraged.

Please read **[CONTRIBUTING.md](CONTRIBUTING.md)** before opening an issue or pull request.

For hardware changes, always identify the exact Waveshare board variant and document any GPIO or electrical assumptions.

For browser-flasher changes, preserve the local/offline image-processing behavior and keep the published assets synchronized with the firmware build.

---

## 📜 License

This project is released under the **MIT License**. See [LICENSE](LICENSE).

Third-party libraries and tools remain under their respective licenses.

---

## 🙏 Credits & references

### Libraries

- **[LovyanGFX](https://github.com/lovyan03/LovyanGFX)** — display driver and graphics
- **[Arduino ESP32](https://github.com/espressif/arduino-esp32)** — ESP32 Arduino core

### Hardware

- **[Waveshare](https://www.waveshare.com/)** — ESP32-S3 display hardware

### Reference project

The documentation style and browser-flashing presentation were influenced by the excellent **[Waveshare ESP32-S3 2inch Capacitive Touch Display — Marble Roller Game](https://github.com/OzInFl/Waveshare-ESP32-S3-2inch-Capacitive-Touch-Display-Marble-Roller-Game)** by **OzInFl**.

That reference project targets the **Touch-LCD-2** board. This repository targets the **No-Touch** board and is a separate implementation.

---

## 📌 Project status

**Stable workflow:**

```text
Select photo
     ↓
Browser prepares 320 × 240 JPEG
     ↓
USB browser flasher
     ↓
ESP32-S3 Flash
     ↓
LovyanGFX + PSRAM
     ↓
Photo on ST7789T3
```

Wi-Fi/iPhone photo transfer is **not part of the current stable firmware**.

---

<div align="center">

### ⭐ Simple setup. Local processing. Tiny photo display.

**[⚡ Launch the Web Flasher](https://chinmayasameeru.github.io/waveshare-photo-display/)**

</div>
