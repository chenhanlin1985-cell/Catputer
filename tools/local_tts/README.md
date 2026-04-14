# Local TTS Notes

This project now reserves a `voice_data` flash partition for future ESP-SR / ESP-TTS
integration on the handheld.

Current status:
- partition table is in `partitions_catputer_local_tts.csv`
- runtime checks are in `src/local_tts.*`
- playback can switch to local TTS once the engine is linked and voice data is flashed

Expected voice data file:
- `tools/local_tts/voice_data/esp_tts_voice_data_xiaoxin.dat`

Flash helper:
- `scripts/windows/flash-local-tts-voice.ps1`

Current limitation:
- the repository does **not** yet vendor Espressif's ESP-SR / ESP-TTS library or the
  `voice_data` blob, so local synthesis remains a scaffold until those assets are added.
