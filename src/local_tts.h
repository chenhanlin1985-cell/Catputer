#pragma once
#include <Arduino.h>
#include <stdint.h>

class LocalTTS {
public:
    void begin();

    bool hasVoicePartition() const { return voicePartitionFound; }
    bool canSynthesize() const { return runtimeReady; }
    const char* availabilityReason() const { return reason; }
    uint32_t getSampleRate() const { return sampleRate; }

    // Minimal integration point for future ESP-SR TTS.
    // Returns true only when local synthesis actually produced PCM.
    bool synthesizeToPCM(const char* text, int16_t* buffer, size_t maxSamples, size_t& outSamples);

private:
    bool voicePartitionFound = false;
    bool runtimeReady = false;
    uint32_t sampleRate = 16000;
    char reason[96] = "";
};
