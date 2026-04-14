#include "pet_storage.h"

#include <FS.h>
#include <time.h>

#if defined(CATPUTER_WAVESHARE_AMOLED_18)
#include <SD_MMC.h>
#include <pin_config.h>
#define PET_FS SD_MMC
#else
#include <SPI.h>
#include <SD.h>
#define PET_FS SD
#endif

namespace {
#if !defined(CATPUTER_WAVESHARE_AMOLED_18)
    constexpr int SD_SPI_SCK_PIN  = 40;
    constexpr int SD_SPI_MISO_PIN = 39;
    constexpr int SD_SPI_MOSI_PIN = 14;
    constexpr int SD_SPI_CS_PIN   = 12;

    SPIClass petStorageSpi(FSPI);
#endif
    bool sdReady = false;

    const char* PET_DIR = "/pet";
    const char* SOUVENIRS_FILE = "/pet/souvenirs.txt";
    const char* EVENTS_FILE = "/pet/events.log";
    const char* MEMORY_DIR = "/pet/memory";

    void ensureDir() {
        if (!PET_FS.exists(PET_DIR)) {
            PET_FS.mkdir(PET_DIR);
        }
        if (!PET_FS.exists(MEMORY_DIR)) {
            PET_FS.mkdir(MEMORY_DIR);
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

    String sanitizePetId(const char* petId) {
        String out = petId ? petId : "";
        if (out.length() == 0) out = "device-cat";
        for (size_t i = 0; i < out.length(); i++) {
            char ch = out[i];
            bool safe = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                        (ch >= '0' && ch <= '9') || ch == '-' || ch == '_';
            if (!safe) out.setCharAt(i, '_');
        }
        return out;
    }

    String memoryPathForPet(const char* petId) {
        return String(MEMORY_DIR) + "/" + sanitizePetId(petId) + ".txt";
    }

    String memoryLogPathForPet(const char* petId) {
        return String(MEMORY_DIR) + "/" + sanitizePetId(petId) + ".log";
    }

    bool rewriteTailFile(const String& path, const String& newLine, uint8_t maxLines) {
        if (maxLines == 0) return false;

        static constexpr uint8_t kMaxBufferedLines = 48;
        if (maxLines > kMaxBufferedLines) maxLines = kMaxBufferedLines;

        String recent[kMaxBufferedLines];
        uint16_t seen = 0;

        if (PET_FS.exists(path)) {
            File readFile = PET_FS.open(path, FILE_READ);
            if (readFile) {
                while (readFile.available()) {
                    String line = readFile.readStringUntil('\n');
                    line.trim();
                    if (line.length() == 0) continue;
                    recent[seen % maxLines] = line;
                    seen++;
                }
                readFile.close();
            }
        }

        recent[seen % maxLines] = newLine;
        seen++;

        if (PET_FS.exists(path)) {
            PET_FS.remove(path);
        }
        File writeFile = PET_FS.open(path, FILE_WRITE);
        if (!writeFile) return false;

        uint8_t available = seen < maxLines ? static_cast<uint8_t>(seen) : maxLines;
        uint16_t start = seen > available ? (seen - available) : 0;
        for (uint8_t i = 0; i < available; i++) {
            uint8_t idx = (start + i) % maxLines;
            writeFile.print(recent[idx]);
            writeFile.print('\n');
        }
        writeFile.close();
        return true;
    }
}

bool PetStorage::begin() {
#if defined(CATPUTER_WAVESHARE_AMOLED_18)
    SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
    if (!SD_MMC.begin("/sdcard", true)) {
        sdReady = false;
        Serial.println("[SD] SD_MMC init failed");
        return false;
    }
#else
    petStorageSpi.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, petStorageSpi, 25000000)) {
        sdReady = false;
        Serial.println("[SD] init failed");
        return false;
    }
#endif

    if (PET_FS.cardType() == CARD_NONE) {
        sdReady = false;
        Serial.println("[SD] no card");
        return false;
    }

    ensureDir();
    sdReady = true;
    Serial.printf("[SD] ready, size=%lluMB\n", PET_FS.cardSize() / (1024ULL * 1024ULL));
    return true;
}

bool PetStorage::isAvailable() {
    return sdReady;
}

fs::FS& PetStorage::fs() {
    return PET_FS;
}

uint64_t PetStorage::cardSizeMB() {
    if (!sdReady) return 0;
    return PET_FS.cardSize() / (1024ULL * 1024ULL);
}

bool PetStorage::loadSouvenirs(char items[][SOUVENIR_ITEM_LEN], char notes[][SOUVENIR_NOTE_LEN], uint8_t& count, uint8_t maxCount) {
    count = 0;
    if (!sdReady || !PET_FS.exists(SOUVENIRS_FILE)) return false;

    File file = PET_FS.open(SOUVENIRS_FILE, FILE_READ);
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
    if (PET_FS.exists(SOUVENIRS_FILE)) {
        PET_FS.remove(SOUVENIRS_FILE);
    }
    File file = PET_FS.open(SOUVENIRS_FILE, FILE_WRITE);
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
    String ts = makeTimestamp();
    String line = ts + " | " + String(eventType ? eventType : "event") + " | " + String(detail ? detail : "");
    return rewriteTailFile(EVENTS_FILE, line, MAX_EVENT_LOG_LINES);
}

bool PetStorage::loadPromptMemory(const char* petId, int askedDayStamp[PROMPT_SLOT_COUNT], char replies[PROMPT_SLOT_COUNT][PROMPT_REPLY_LEN]) {
    for (uint8_t i = 0; i < PROMPT_SLOT_COUNT; i++) {
        askedDayStamp[i] = -1;
        replies[i][0] = '\0';
    }
    if (!sdReady) return false;

    String path = memoryPathForPet(petId);
    if (!PET_FS.exists(path)) return false;

    File file = PET_FS.open(path, FILE_READ);
    if (!file) return false;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;
        int first = line.indexOf('\t');
        int second = first >= 0 ? line.indexOf('\t', first + 1) : -1;
        if (first <= 0 || second <= first) continue;

        int slot = line.substring(0, first).toInt();
        if (slot < 0 || slot >= PROMPT_SLOT_COUNT) continue;
        askedDayStamp[slot] = line.substring(first + 1, second).toInt();
        copyField(replies[slot], PROMPT_REPLY_LEN, line.substring(second + 1));
    }

    file.close();
    return true;
}

bool PetStorage::savePromptMemory(const char* petId, const int askedDayStamp[PROMPT_SLOT_COUNT], const char replies[PROMPT_SLOT_COUNT][PROMPT_REPLY_LEN]) {
    if (!sdReady) return false;
    ensureDir();

    String path = memoryPathForPet(petId);
    if (PET_FS.exists(path)) {
        PET_FS.remove(path);
    }
    File file = PET_FS.open(path, FILE_WRITE);
    if (!file) return false;

    for (uint8_t i = 0; i < PROMPT_SLOT_COUNT; i++) {
        file.print(i);
        file.print('\t');
        file.print(askedDayStamp[i]);
        file.print('\t');
        file.print(replies[i]);
        file.print('\n');
    }

    file.close();
    return true;
}

bool PetStorage::appendPetMemoryEvent(const char* petId, const char* eventType, const char* detail) {
    if (!sdReady) return false;
    ensureDir();

    String path = memoryLogPathForPet(petId);
    String ts = makeTimestamp();
    String line = ts + " | " + String(eventType ? eventType : "memory") + " | " + String(detail ? detail : "");
    return rewriteTailFile(path, line, MAX_PET_MEMORY_LINES);
}

bool PetStorage::loadRecentPetMemoryEvents(const char* petId, char lines[][96], uint8_t& count, uint8_t maxCount) {
    count = 0;
    if (!sdReady) return false;

    String path = memoryLogPathForPet(petId);
    if (!PET_FS.exists(path)) return false;

    File file = PET_FS.open(path, FILE_READ);
    if (!file) return false;

    static constexpr uint8_t kMaxBuffer = 12;
    String recent[kMaxBuffer];
    uint8_t seen = 0;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        recent[seen % kMaxBuffer] = line;
        seen++;
    }
    file.close();

    uint8_t available = seen < kMaxBuffer ? seen : kMaxBuffer;
    if (available == 0) return false;
    uint8_t start = seen > available ? (seen - available) : 0;
    for (uint8_t i = 0; i < available && count < maxCount; i++) {
        uint8_t idx = (start + i) % kMaxBuffer;
        copyField(lines[count], 96, recent[idx]);
        count++;
    }
    return count > 0;
}
