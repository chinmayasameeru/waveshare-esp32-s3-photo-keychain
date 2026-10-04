# Waveshare ESP32-S3 Photo Display

Photo-display firmware for the Waveshare ESP32-S3-LCD-2 (No Touch).

Hardware target:
- ESP32-S3R8
- 16 MB external Flash
- 8 MB OPI PSRAM
- 240x320 ST7789T3 LCD
- LCD SCLK 39, MOSI 38, MISO 40, DC 42, CS 45, RST -1, BL 1

Features:
- JPEG photo upload from a phone browser
- Wi-Fi access point mode
- JPEG stored in FFat
- Photo shown on the LCD after upload
- Built-in screen when no photo is stored
- Direct browser flashing through ESP Web Tools

The firmware is compiled for the app3M_fat9M_16MB partition scheme on 16 MB Flash.

Phone use:
1. Flash the firmware.
2. Connect the iPhone to the PhotoDisplay-XXXXXX Wi-Fi network.
3. Password: photo1234
4. Open http://192.168.4.1 in Safari.
5. Pick a JPEG and press Show photo.

This project uses LovyanGFX for the ST7789 display path, following the rendering approach used by the Marble Roller reference project.
