#pragma once
#include <M5Cardputer.h>
#include "utils.h"

class TTSPlayback {
public:
    void begin(const String& host, const String& port,
               int16_t* sharedBuffer, size_t maxSamples);
    void setBuffer(int16_t* sharedBuffer, size_t maxSamples);

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

    static constexpr uint32_t SAMPLE_RATE = 8000;  // 8kHz: 160KB buffer = 10s
    static constexpr int INPUT_BAR_H = 16;
    static constexpr unsigned long STOP_COOLDOWN_MS = 50;  // DMA drain guard

    unsigned long stopTime = 0;

    // Download PCM from TTS proxy into buffer. Returns number of samples read.
    size_t downloadPCM(const char* text);
};
