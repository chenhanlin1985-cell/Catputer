#pragma once
#if defined(CATPUTER_WAVESHARE_AMOLED_18)
#include "waveshare_device.h"
#elif defined(CATPUTER_TOUCH_UI)
#include <M5Unified.h>
#else
#include <M5Cardputer.h>
#endif
#include "utils.h"

class LocalTTS;

class TTSPlayback {
public:
    void begin(const String& host, const String& port,
               int16_t* sharedBuffer, size_t maxSamples);
    void setBuffer(int16_t* sharedBuffer, size_t maxSamples);
    void attachLocalTTS(LocalTTS* localEngine);

    // Download PCM from proxy and start playback. Blocks during download,
    // playback is non-blocking (DMA queue via Speaker.playRaw).
    bool requestAndPlay(const char* text);

    // True while Speaker is still playing audio
    bool isPlaying() const;

    // Stop playback immediately
    void stop();

    // Draw "Speaking..." indicator bar (call during download wait)
    void drawSpeakingBar(M5Canvas& canvas);

private:
    String ttsHost;
    String ttsPort;
    int16_t* buffer = nullptr;
    size_t maxSamples = 0;
    LocalTTS* local = nullptr;
    uint32_t lastSampleRate = SAMPLE_RATE;

    static constexpr uint32_t SAMPLE_RATE = 8000;  // 8kHz: 160KB buffer = 10s
    static constexpr int INPUT_BAR_H = 16;
    static constexpr unsigned long STOP_COOLDOWN_MS = 50;  // DMA drain guard

    unsigned long stopTime = 0;

    // Download PCM from TTS proxy into buffer. Returns number of samples read.
    size_t downloadPCM(const char* text);
};
