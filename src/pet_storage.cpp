#include "pet_storage.h"

#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <time.h>

namespace {
    constexpr int SD_SPI_SCK_PIN  = 40;
    constexpr int SD_SPI_MISO_PIN = 39;
    constexpr int SD_SPI_MOSI_PIN = 14;
    constexpr int SD_SPI_CS_PIN   = 12;

    SPIClass petStorageSpi(FSPI);
    bool sdReady = false;

    const char* PET_DIR = "/pet";
    const char* SOUVENIRS_FILE = "/pet/souvenirs.txt";
    const char* EVENTS_FILE = "/pet/events.log";

    void ensureDir() {
        if (!SD.exists(PET_DIR)) {
            SD.mkdir(PET_DIR);
        }
    }

    void copyField(char* dst, size_t dstSize, const String& src) {
        if (dstSize == 0) return;
        strncpy(dst, src.c_str(), dstSize - 1);
        dst[dstSize - 1] = '\0';
    }

    bool looksCorrupted(const String& value) {
        return value.indexOf('?') >= 0 || value.indexOf("\xEF\xBF\xBD") >= 0;
    }

    String makeTimestamp() {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 0)) {
            char buf[24];
            snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            return String(buf);
        }
        return String(millis());
    }
}

bool PetStorage::begin() {
    petStorageSpi.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, petStorageSpi, 25000000)) {
        sdReady = false;
        Serial.println("[SD] init failed");
        return false;
    }

    if (SD.cardType() == CARD_NONE) {
        sdReady = false;
        Serial.println("[SD] no card");
        return false;
    }

    ensureDir();
    sdReady = true;
    Serial.printf("[SD] ready, size=%lluMB\n", SD.cardSize() / (1024ULL * 1024ULL));
    return true;
}

bool PetStorage::isAvailable() {
    return sdReady;
}

bool PetStorage::loadSouvenirs(char items[][SOUVENIR_ITEM_LEN], char notes[][SOUVENIR_NOTE_LEN], uint8_t& count, uint8_t maxCount) {
    count = 0;
    if (!sdReady || !SD.exists(SOUVENIRS_FILE)) return false;

    File file = SD.open(SOUVENIRS_FILE, FILE_READ);
    if (!file) return false;
    bool sanitized = false;

    while (file.available() && count < maxCount) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        int sep = line.indexOf('\t');
        String item = sep >= 0 ? line.substring(0, sep) : line;
        String note = sep >= 0 ? line.substring(sep + 1) : "";

        if (looksCorrupted(item)) {
            item = "old souvenir";
            sanitized = true;
        }
        if (looksCorrupted(note)) {
            note = "old note";
            sanitized = true;
        }

        copyField(items[count], SOUVENIR_ITEM_LEN, item);
        copyField(notes[count], SOUVENIR_NOTE_LEN, note);
        count++;
    }

    file.close();
    if (sanitized) {
        saveSouvenirs(items, notes, count, maxCount);
    }
    return count > 0;
}

bool PetStorage::saveSouvenirs(const char items[][SOUVENIR_ITEM_LEN], const char notes[][SOUVENIR_NOTE_LEN], uint8_t count, uint8_t maxCount) {
    if (!sdReady) return false;

    ensureDir();
    if (SD.exists(SOUVENIRS_FILE)) {
        SD.remove(SOUVENIRS_FILE);
    }
    File file = SD.open(SOUVENIRS_FILE, FILE_WRITE);
    if (!file) return false;

    uint8_t cappedCount = count > maxCount ? maxCount : count;
    for (uint8_t i = 0; i < cappedCount; i++) {
        file.print(items[i]);
        file.print('\t');
        file.print(notes[i]);
        file.print('\n');
    }

    file.close();
    return true;
}

bool PetStorage::appendEventLog(const char* eventType, const char* detail) {
    if (!sdReady) return false;

    ensureDir();
    File file = SD.open(EVENTS_FILE, FILE_APPEND);
    if (!file) return false;

    String ts = makeTimestamp();
    file.print(ts);
    file.print(" | ");
    file.print(eventType ? eventType : "event");
    file.print(" | ");
    file.print(detail ? detail : "");
    file.print('\n');
    file.close();
    return true;
}
