# Contributing to Catputer

Thanks for your interest in improving Catputer.

## Development Setup

1. Hardware: M5Stack Cardputer / Cardputer Adv
2. Toolchain: [PlatformIO](https://platformio.org/) CLI or VS Code extension
3. Build:
   ```bash
   cp .env.example .env
   source .env
   pio run -t upload
   ```

## Project Structure

```text
src/
|- main.cpp              # App flow, setup mode, WiFi, online service init
|- companion.h/cpp       # Pet logic, animation, offline systems, weather scene
|- chat.h/cpp            # Chat UI and streamed response display
|- sprites.h             # 16x16 sprite data
|- ai_client.h/cpp       # Direct OpenAI-compatible chat client
|- voice_input.h/cpp     # Push-to-talk voice capture
|- tts_playback.h/cpp    # Speaker playback for TTS audio
|- weather_client.h/cpp  # Open-Meteo weather client
|- config.h/cpp          # Persistent config and pet save data
|- cmd_server.h/cpp      # Local command server
|- state_broadcast.h/cpp # UDP state broadcast
|- utils.h               # Shared helpers
tools/                   # STT/TTS relay and helpers
scripts/                 # Windows helper scripts
docs/                    # Notes, plans, architecture docs
```

## Guidelines

- Keep features small and readable. The device has limited RAM and a small screen.
- Avoid heap-heavy code in hot paths. Prefer fixed buffers and `snprintf` in render/update loops.
- Do not hardcode real secrets in tracked files. Use `${sysenv.VAR}` in `platformio.ini`.
- Test on real hardware when possible. At minimum, run `pio run` before submitting changes.

## Reporting Bugs

Include:

- What you expected
- What actually happened
- Reproduction steps
- Serial output when relevant

## Pull Requests

Before opening a PR:

1. Make sure the firmware still builds with `pio run`
2. Keep scope focused
3. Explain the user-facing change clearly
