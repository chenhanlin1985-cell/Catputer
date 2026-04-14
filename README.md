# Catputer

[中文说明](README_CN.md)

Catputer is a pocket companion project built around a touchscreen ESP32-S3 pet device, a PC cat house, and SD-card based resources. The current product direction is no longer “a tiny chat terminal”; it is a small pet that appears to live in its own room, remembers recent moments, and uses AI only when it can make the interaction feel more alive.

The active endpoint is the Waveshare ESP32-S3 Touch AMOLED 1.8. The older M5Stack Cardputer / Cardputer Adv build remains in the repository, but is now in maintenance/archive mode.

## Current Shape

- **Waveshare touchscreen endpoint**: the main pet device, with a room background, touch interactions, weather/time display, SD resources, SD OTA, and attention-session bubbles.
- **PC cat house**: a Godot desktop companion manager for multiple pets, device transfer, memory review, and future AI world-event generation.
- **Cardputer endpoint**: preserved for existing keyboard-centric experiments, but not the current product focus.

## What Makes It Fun

Catputer tries to make the pet feel like it has a tiny life outside the user’s direct control:

- It can move around a small room instead of sitting in a static UI.
- Touching the pet after a period of absence can trigger a “reunion” moment with recent little events.
- Weather, time of day, memories, souvenirs, and pet state can feed short contextual bubbles.
- Online AI is used as a small-world event writer, not just as a generic chat bot.
- Offline fallback text still works, so the pet does not become silent without network access.

## Main Features

- Touch-first pet home UI for Waveshare AMOLED.
- Persistent pet stats: fullness, mood, energy, cleanliness, and bond.
- Multiple pet appearances, including cat and penguin resources.
- SD card resources for dialogue, backgrounds, photos, IME resources, and OTA firmware.
- Time-based room backgrounds and cached weather/time display.
- Attention-session interaction: reunion text plus recent event/memory context.
- AI touch bubbles when Wi-Fi and API configuration are available.
- PC cat house for multi-pet management and endpoint transfer.
- SD OTA: copy firmware to the card and update from the device settings page.

## Setup

Do not commit real Wi-Fi credentials, API keys, or personal tokens. Use:

- `.env.example`
- `clawputer.local.ps1.example`

Common variables:

- `WIFI_SSID`
- `WIFI_PASS`
- `API_KEY`
- `API_HOST`
- `API_PORT`
- `DEFAULT_CITY`

## Build

The default development target is now the Waveshare touchscreen endpoint:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\pio-run.ps1
```

Explicit target:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\pio-run.ps1 -EnvName waveshare-amoled-18
```

Cardputer can still be built explicitly:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\pio-run.ps1 -EnvName m5stack-cardputer
```

## Flashing Waveshare

Use the documented COM9 wait flow. Do not flash the runtime COM8 port directly.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\flash-waveshare-com9-wait.ps1 -AcknowledgeChecklist -WaitSeconds 35 -TouchAttempts 3
```

Read before troubleshooting:

- [Waveshare flash must-read](docs/flash-must-read.md)
- [Flashing and networking SOP](docs/刷机与联网排查SOP.md)

## SD OTA

Prepare the OTA package:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows\prepare-sd-ota.ps1
```

Copy the generated file to the SD card:

```text
/firmware/update.bin
```

Then open the touchscreen settings page and trigger SD update.

## Recommended Docs

- [Current product state and setup](docs/current-product-state-and-setup.md)
- [Touch-first AI world roadmap](docs/touch-first-ai-world-roadmap.md)
- [Attention session design](docs/attention-session-emotional-design.md)
- [PlatformIO cache isolation](docs/platformio-cache-isolation.md)
- [BLE and touchscreen troubleshooting](docs/ble-keyboard-troubleshooting.md)

## Repo Notes

- Keep secrets out of git.
- Keep generated firmware, logs, and local scratch files out of git.
- New features should update the relevant docs in the same change.
- Avoid reintroducing local TTS on Waveshare until partitioning and memory isolation are redesigned.
