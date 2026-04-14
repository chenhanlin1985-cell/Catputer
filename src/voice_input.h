#pragma once
#if defined(CATPUTER_WAVESHARE_AMOLED_18)
#include "waveshare_device.h"
#elif defined(CATPUTER_TOUCH_UI)
#include <M5Unified.h>
#else
#include <M5Cardputer.h>
#endif
#include "utils.h"

class VoiceInput {
public:
    void begin(const String& sttHost, const String& sttPort);
    bool ensureReady();
    void releaseIfIdle();

    // Push-to-talk: call when Fn pressed/released
    void startRecording();
    bool stopRecording();  // returns true if STT result available

    bool isRecording() const { return recording; }
    bool isTranscribing() const { return transcribing; }

    // Get transcription result (clears after call)
    String takeResult();

    // Expose buffer for TTS reuse (recording and playback never overlap)
    int16_t* getBuffer() const { return recordBuffer; }
    size_t getMaxSamples() const { return maxSamples; }

    // Recording duration in seconds
    float getRecordingDuration() const;

    // Draw recording indicator over the input bar area
    void drawRecordingBar(M5Canvas& canvas);
    void drawTranscribingBar(M5Canvas& canvas);

private:
    String sttHostStr;
    String sttPortStr;

    int16_t* recordBuffer = nullptr;
    size_t maxSamples = 0;
    size_t samplesRecorded = 0;
    bool recording = false;
    bool transcribing = false;
    unsigned long recordStartTime = 0;
    String result;

    static constexpr uint32_t SAMPLE_RATE = 16000;
    static constexpr float MAX_RECORD_SEC = 5.0f;  // 5s * 16kHz * 2B = 160KB; TTS @ 8kHz = 10s
    static constexpr float MIN_RECORD_SEC = 0.3f;
    static constexpr int INPUT_BAR_H = 16;

    void initMic();
    void deinitMic();
    bool allocBuffer();
    void freeBuffer();
    bool hasMeaningfulAudio(const int16_t* data, size_t sampleCount) const;
    String sendToSTT(const int16_t* data, size_t sampleCount);
    void writeWavHeader(uint8_t* header, uint32_t dataSize);
};
