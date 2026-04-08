#pragma once

#include <Arduino.h>

namespace PetStorage {
    static constexpr uint8_t MAX_SOUVENIRS = 12;
    static constexpr size_t SOUVENIR_ITEM_LEN = 48;
    static constexpr size_t SOUVENIR_NOTE_LEN = 96;

    bool begin();
    bool isAvailable();

    bool loadSouvenirs(char items[][SOUVENIR_ITEM_LEN], char notes[][SOUVENIR_NOTE_LEN], uint8_t& count, uint8_t maxCount = MAX_SOUVENIRS);
    bool saveSouvenirs(const char items[][SOUVENIR_ITEM_LEN], const char notes[][SOUVENIR_NOTE_LEN], uint8_t count, uint8_t maxCount = MAX_SOUVENIRS);

    bool appendEventLog(const char* eventType, const char* detail);
}
