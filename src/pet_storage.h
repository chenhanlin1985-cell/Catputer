#pragma once

#include <Arduino.h>
#include <FS.h>

namespace PetStorage {
    static constexpr uint8_t MAX_SOUVENIRS = 12;
    static constexpr size_t SOUVENIR_ITEM_LEN = 48;
    static constexpr size_t SOUVENIR_NOTE_LEN = 96;
    static constexpr uint8_t PROMPT_SLOT_COUNT = 6;
    static constexpr size_t PROMPT_REPLY_LEN = 64;
    static constexpr uint8_t MAX_PET_MEMORY_LINES = 24;
    static constexpr uint8_t MAX_EVENT_LOG_LINES = 40;

    bool begin();
    bool isAvailable();
    fs::FS& fs();
    uint64_t cardSizeMB();

    bool loadSouvenirs(char items[][SOUVENIR_ITEM_LEN], char notes[][SOUVENIR_NOTE_LEN], uint8_t& count, uint8_t maxCount = MAX_SOUVENIRS);
    bool saveSouvenirs(const char items[][SOUVENIR_ITEM_LEN], const char notes[][SOUVENIR_NOTE_LEN], uint8_t count, uint8_t maxCount = MAX_SOUVENIRS);

    bool appendEventLog(const char* eventType, const char* detail);
    bool loadPromptMemory(const char* petId, int askedDayStamp[PROMPT_SLOT_COUNT], char replies[PROMPT_SLOT_COUNT][PROMPT_REPLY_LEN]);
    bool savePromptMemory(const char* petId, const int askedDayStamp[PROMPT_SLOT_COUNT], const char replies[PROMPT_SLOT_COUNT][PROMPT_REPLY_LEN]);
    bool appendPetMemoryEvent(const char* petId, const char* eventType, const char* detail);
    bool loadRecentPetMemoryEvents(const char* petId, char lines[][96], uint8_t& count, uint8_t maxCount = 6);
}
