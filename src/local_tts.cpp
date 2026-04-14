#include "local_tts.h"
#include <esp_partition.h>
#include <esp_idf_version.h>
#include <esp_spi_flash.h>
#include <esp_heap_caps.h>

#if defined(CATPUTER_ENABLE_LOCAL_ESP_TTS) && __has_include("esp_tts.h")
#define CATPUTER_HAS_ESP_SR_TTS 1
#include "esp_tts.h"
#include "esp_tts_voice_xiaole.h"
#else
#define CATPUTER_HAS_ESP_SR_TTS 0
#endif

#if CATPUTER_HAS_ESP_SR_TTS
static esp_tts_voice_t* gVoice = nullptr;
static esp_tts_handle_t gHandle = nullptr;

static bool rebuildTTSHandle() {
    if (!gVoice) return false;
    if (gHandle) {
        esp_tts_destroy(gHandle);
        gHandle = nullptr;
    }
    gHandle = esp_tts_create(gVoice);
    return gHandle != nullptr;
}
#endif

void LocalTTS::begin() {
    voicePartitionFound = false;
    runtimeReady = false;
    sampleRate = 16000;
    strncpy(reason, "local tts unavailable", sizeof(reason) - 1);
    reason[sizeof(reason) - 1] = '\0';

    const esp_partition_t* voicePartition =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "voice_data");
    if (!voicePartition) {
        strncpy(reason, "voice_data partition missing", sizeof(reason) - 1);
        reason[sizeof(reason) - 1] = '\0';
        Serial.println("[LOCAL_TTS] voice_data partition not found");
        return;
    }

    voicePartitionFound = true;
    Serial.printf("[LOCAL_TTS] voice_data partition found: offset=0x%08x size=%u\n",
                  (unsigned)voicePartition->address, (unsigned)voicePartition->size);

#if CATPUTER_HAS_ESP_SR_TTS
    const void* voicedata = nullptr;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_partition_mmap_handle_t mmapHandle;
    esp_err_t err = esp_partition_mmap(voicePartition, 0, voicePartition->size,
                                       ESP_PARTITION_MMAP_DATA, &voicedata, &mmapHandle);
#else
    spi_flash_mmap_handle_t mmapHandle;
    esp_err_t err = esp_partition_mmap(voicePartition, 0, voicePartition->size,
                                       SPI_FLASH_MMAP_DATA, &voicedata, &mmapHandle);
#endif
    if (err != ESP_OK || !voicedata) {
        strncpy(reason, "voice_data mmap failed", sizeof(reason) - 1);
        reason[sizeof(reason) - 1] = '\0';
        Serial.printf("[LOCAL_TTS] voice_data mmap failed: %d\n", (int)err);
        return;
    }

    gVoice = esp_tts_voice_set_init(&esp_tts_voice_xiaole, (void*)voicedata);
    if (!gVoice) {
        strncpy(reason, "voice set init failed", sizeof(reason) - 1);
        reason[sizeof(reason) - 1] = '\0';
        Serial.println("[LOCAL_TTS] voice set init failed");
        return;
    }

    if (!rebuildTTSHandle()) {
        strncpy(reason, "tts create failed", sizeof(reason) - 1);
        reason[sizeof(reason) - 1] = '\0';
        Serial.println("[LOCAL_TTS] esp_tts_create failed");
        return;
    }

    runtimeReady = true;
    strncpy(reason, "ready", sizeof(reason) - 1);
    reason[sizeof(reason) - 1] = '\0';
    Serial.println("[LOCAL_TTS] Local ESP-SR TTS ready");
#else
    strncpy(reason, "esp-sr tts library not linked yet", sizeof(reason) - 1);
    reason[sizeof(reason) - 1] = '\0';
    Serial.println("[LOCAL_TTS] ESP-SR TTS library not linked yet");
#endif
}

bool LocalTTS::synthesizeToPCM(const char* text, int16_t* buffer, size_t maxSamples, size_t& outSamples) {
    outSamples = 0;
    if (!text || !text[0] || !buffer || maxSamples == 0) return false;
    if (!voicePartitionFound) return false;
    if (!runtimeReady) {
        Serial.printf("[LOCAL_TTS] Synthesis unavailable: %s\n", reason);
        return false;
    }

#if CATPUTER_HAS_ESP_SR_TTS
    uint32_t heapBefore = ESP.getFreeHeap();
    uint32_t largestBefore = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    Serial.printf("[LOCAL_TTS] Begin synth, heap=%u largest=%u text=%u\n",
                  heapBefore, largestBefore, (unsigned)strlen(text));

    if (!esp_tts_parse_chinese(gHandle, text)) {
        Serial.println("[LOCAL_TTS] esp_tts_parse_chinese failed");
        return false;
    }

    size_t totalSamples = 0;
    int chunkLen = 0;
    do {
        short* pcm = esp_tts_stream_play(gHandle, &chunkLen, 3);
        if (chunkLen <= 0 || !pcm) break;

        size_t chunkSamples = (size_t)chunkLen;
        if (totalSamples + chunkSamples > maxSamples) {
            chunkSamples = maxSamples - totalSamples;
        }
        if (chunkSamples > 0) {
            memcpy(buffer + totalSamples, pcm, chunkSamples * sizeof(int16_t));
            totalSamples += chunkSamples;
        }
        if (totalSamples >= maxSamples) break;
    } while (chunkLen > 0);

    esp_tts_stream_reset(gHandle);
    outSamples = totalSamples;
    uint32_t heapAfter = ESP.getFreeHeap();
    uint32_t largestAfter = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    Serial.printf("[LOCAL_TTS] Synthesized %u samples, heap=%u largest=%u\n",
                  (unsigned)outSamples, heapAfter, largestAfter);

    if (!rebuildTTSHandle()) {
        runtimeReady = false;
        strncpy(reason, "tts recreate failed", sizeof(reason) - 1);
        reason[sizeof(reason) - 1] = '\0';
        Serial.println("[LOCAL_TTS] esp_tts handle recreate failed");
    }
    return outSamples > 0;
#else
    return false;
#endif
}
