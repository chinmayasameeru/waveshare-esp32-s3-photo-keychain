# User Manual

## What you need

- Waveshare ESP32-S3 2-inch No-Touch Display Development Board
- USB-C data cable
- Desktop Chrome or Edge for initial firmware installation
- iPhone or another Wi-Fi-capable phone for photo transfer

## First installation

Open the Photo Keychain installer:

**https://chinmayasameeru.github.io/waveshare-esp32-s3-photo-keychain/**

Then:

1. Connect the ESP32-S3 with a USB-C data cable.
2. Open the installer in desktop Chrome or Edge.
3. Press **Flash firmware**.
4. Choose the ESP32-S3 serial port.
5. Wait for the firmware to finish installing.
6. Let the board reboot.
7. Disconnect USB for standalone operation.

Web Serial is used only for this installation step.

## Add a photo

1. Press the physical **BOOT** button.
2. Wait for the LCD to show the PhotoKeychain network details.
3. On the phone, join Wi-Fi network **PhotoKeychain**.
4. Enter password **photo1234**.
5. Open **http://192.168.4.1**.
6. Tap **Choose Photo**.
7. Select an image.
8. Tap **Save Photo to Keychain**.
9. The LCD updates to the saved image.

## Replacing a photo

A firmware re-flash is not required.

Repeat the photo workflow whenever you want to replace the stored image.

## Wi-Fi behavior

The access point is intentionally temporary.

- SSID: `PhotoKeychain`
- Password: `photo1234`
- Address: `192.168.4.1`
- Inactivity timeout: 5 minutes

Activity from the phone resets the inactivity timer. An active upload is allowed to complete.

## Normal operation

When Wi-Fi is off, the stored photo remains on the LCD.

The firmware enters light sleep after approximately 30 seconds of normal-mode inactivity and uses the BOOT GPIO as the wake source.

## Recovery

### Reinstall firmware

Use the browser installer again.

### USB download mode

When the browser cannot connect to the board, use the ESP32-S3 BOOT and RESET/EN controls to enter download mode, then retry from desktop Chrome or Edge.

### Wi-Fi setup page unavailable

Confirm that:

- the phone is connected to PhotoKeychain;
- the local address is exactly `http://192.168.4.1`;
- the board has not already reached the five-minute inactivity timeout.

## Scope of this release

The stable release is intentionally photo-only. It does not include crop editing, GIF, video, or other media playback features.
