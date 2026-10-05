# Contributing to Waveshare ESP32-S3 Photo Keychain

Thank you for contributing to the project.

The stable primary target is the **Waveshare ESP32-S3 2-inch No-Touch Display Development Board** running Photo Keychain firmware.

The repository also contains the original static USB photo-display implementation under [`PhotoDisplay-Legacy/`](PhotoDisplay-Legacy/). Treat that as a secondary reference project.

## Before you contribute

Confirm the exact hardware variant before changing LCD, GPIO, Flash, PSRAM, or power behavior.

The Touch-LCD-2 hardware is not interchangeable with the No-Touch board used by the primary project.

## Development workflow

1. Create a focused branch from `main`.
2. Make the smallest change that solves the problem.
3. Keep firmware, browser behavior, and documentation aligned.
4. Build with the documented Arduino CLI toolchain.
5. Test on real hardware whenever a change affects display, Wi-Fi, storage, power, or USB flashing.
6. Update the relevant documentation.
7. Open a pull request with the change, rationale, test method, and any known limitations.

## Firmware guidelines

- Preserve the documented No-Touch GPIO mapping unless a hardware change is intentional.
- Keep the ST7789T3/LovyanGFX configuration consistent with the supported board.
- Validate incoming photo data before making it the active photo.
- Keep uploads transactional by using the temporary-file flow.
- Keep Wi-Fi temporary and scoped to the local phone workflow.
- Avoid unnecessary services, dependencies, and background work.
- Keep user-visible error states understandable on the LCD.

## Browser installer guidelines

- Keep image preparation local to the browser.
- Preserve Web Serial support for the documented desktop browsers.
- Do not upload user photos to third-party services.
- Keep firmware filenames and flash addresses synchronized with the CI pipeline.
- Treat `docs/` as generated deployment output rather than source.

## Documentation

Update documentation when you change:

- supported hardware;
- GPIO mappings;
- partitioning or storage;
- user workflow;
- Wi-Fi behavior;
- build requirements;
- release/deployment behavior.

The documentation set is:

- [Pipeline](documentation/PIPELINE.md)
- [Hardware](documentation/HARDWARE.md)
- [Firmware Architecture](documentation/FIRMWARE-ARCHITECTURE.md)
- [User Manual](documentation/USER-MANUAL.md)

## Pull requests

A strong pull request explains:

- **What changed**
- **Why it changed**
- **Hardware tested**
- **Browser tested**
- **CI/build result**
- **Known limitations**

For user-visible changes, describe the before/after behavior.

## Issue reports

Include enough detail to reproduce the problem:

- exact board variant;
- operating system;
- browser and version;
- exact steps;
- expected behavior;
- actual behavior;
- serial output or screenshots when available.

## Commit messages

Prefer concise, imperative messages such as:

- `Fix photo upload timeout`
- `Improve AP shutdown handling`
- `Document LCD pin mapping`

## Releases and generated files

GitHub Actions builds the stable firmware and publishes the browser installer.

Do not hand-edit generated binaries under `docs/`.

Source changes normally belong under:

- `PhotoKeychain/`
- `web/`
- `documentation/`
- `.github/`
- `README.md`

## License

By contributing, you agree that your contributions are licensed under the repository's [MIT License](LICENSE).
