# Firmware Architecture

## Runtime model

Photo Keychain is intentionally a small single-sketch Arduino firmware.

Primary source:

`PhotoKeychain/PhotoKeychain.ino`

The firmware owns four concerns:

1. LCD initialization and rendering
2. persistent photo storage
3. temporary phone-access Wi-Fi
4. power/idle state transitions

## Boot sequence

```text
Power-on
  ↓
GPIO + LCD initialization
  ↓
Wi-Fi OFF
  ↓
Mount FFat
  ↓
Does /photo.jpg exist?
  ├─ yes → allocate PSRAM buffer → read → JPEG decode → LCD
  └─ no  → READY screen
  ↓
Watch BOOT button
```

## Photo upload sequence

```text
BOOT
 ↓
Wi-Fi AP starts
 ↓
HTTP server starts
 ↓
Phone requests /
 ↓
Phone chooses a photo
 ↓
POST /save
 ↓
/photo.tmp created
 ↓
HTTP upload chunks written
 ↓
flush + close
 ↓
/photo.jpg replaced
 ↓
photo rendered immediately
```

The temporary file protects the last known-good image from partial transfers.

## HTTP endpoints

| Endpoint | Method | Purpose |
|---|---|---|
| `/` | GET | Serve the phone setup page |
| `/status` | GET | Return basic runtime status |
| `/save` | POST | Receive the photo upload |

The firmware also redirects unknown requests to the local root address.

## Storage lifecycle

### Startup

`beginStorage()` mounts FFat and records whether storage is ready.

### Upload

The browser sends a JPEG as multipart form data. The firmware writes it to `/photo.tmp`.

### Commit

After the upload finishes:

1. flush;
2. close;
3. validate size;
4. remove the previous `/photo.jpg`;
5. rename `/photo.tmp` to `/photo.jpg`.

### Display

The JPEG is copied into PSRAM and decoded using LovyanGFX.

## State and timing

Key runtime state includes:

- `apRunning`
- `uploadInProgress`
- `photoWriteOk`
- `lastActivityMs`
- `normalModeSinceMs`

Two user-visible timers define behavior:

- **5 minutes:** AP inactivity timeout;
- **30 seconds:** normal-mode delay before light sleep.

## Power management

The device keeps Wi-Fi off during normal operation.

Pressing BOOT temporarily starts the AP. After inactivity, the AP is stopped and Wi-Fi is turned off. When normal mode has been idle long enough, the ESP32 enters light sleep and can wake through GPIO0.

## Design principles

### Local-first

The phone communicates directly with the ESP32. No application account, cloud service, or backend is required.

### Failure containment

Upload data is staged in a temporary file. Invalid files are rejected before becoming the active photo.

### Minimal runtime

The firmware does not keep unnecessary network services or background media decoders running.

### Observable behavior

The LCD provides simple state screens for ready mode, AP mode, and error cases, while serial logging provides deeper diagnostics during development.
