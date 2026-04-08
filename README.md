# Orange Cat Pocket Pet

[中文说明](README_CN.md)

A pocket pet for `M5Stack Cardputer / Cardputer Adv`.

This version focuses on a small orange cat that works well offline and gets smarter online.

## Features

- Offline pet loop: feed, play, nap, clean, outing, mini-game
- Persistent pet stats: fullness, mood, energy, cleanliness, bond
- Chinese-first UI
- Direct chat to an OpenAI-compatible API
- Local PC relay for STT and TTS
- SD card support for souvenirs, photos, and event logs

## Runtime Model

The project is split into two layers:

1. Offline pocket pet
2. Online enhancements

Offline mode keeps the pet playable even without network access.

Online mode adds:

- AI chat
- voice input
- voice playback
- weather sync

## Setup

### Environment

Use either `.env` or `clawputer.local.ps1`.

Required:

- `WIFI_SSID`
- `WIFI_PASS`
- `API_KEY`

Common defaults:

- `API_HOST=dashscope.aliyuncs.com`
- `API_PORT=443`
- `DEFAULT_CITY=Shenzhen`

### Build and Flash

```bash
pio run -t upload
```

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\flash.ps1 -Port COM5
```

### Voice Relay

Text chat does not require a local gateway.

If you want STT / TTS:

```bash
python tools/stt_proxy.py
```

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\start-stt-proxy.ps1
```

## Controls

### Pet Mode

- `TAB`: open chat
- `, / ; .`: move
- `f`: feed
- `p`: play
- `n`: nap
- `c`: clean
- `g`: mini-game
- `o`: outing
- `v`: souvenir box
- `h`: help panel
- `i`: status panel
- `Fn + ,` / `Fn + .`: volume down / up
- `Fn + R`: reset config

### Chat Mode

- `TAB`: return to pet
- `Enter`: send
- `Backspace`: delete
- `Fn`: push to talk
- `Fn + ;` / `Fn + /`: scroll
- `Ctrl + Enter`: toggle auto-read

## Local Services

Still needed for voice:

- `tools/stt_proxy.py`
- `ffmpeg`
- `edge-tts`
- `GROQ_API_KEY`

Not required anymore:

- `OpenClaw`

## Storage

With an SD card:

- souvenirs are stored under `/pet/souvenirs.txt`
- event logs are appended to `/pet/events.log`
- outing photos are read from `/pet/photos/`

Without an SD card, the project falls back to NVS for core save data.

## Repo Notes

- Do not commit real Wi-Fi credentials or API keys.
- Use the example config files as templates.
- Local logs, test payloads, and personal helper files are intentionally ignored.
