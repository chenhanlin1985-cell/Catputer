#include "pet_dialogue.h"

#include <SD.h>

#include "pet_storage.h"

namespace {
    static constexpr const char* DIALOGUE_PATH = "/pet/dialogue/dialogue.txt";
    static constexpr int MAX_DIALOGUE_ENTRIES = 64;

    struct DialogueEntry {
        String key;
        String values;
    };

    DialogueEntry gEntries[MAX_DIALOGUE_ENTRIES];
    int gEntryCount = 0;
    bool gInitialized = false;
    bool gUsingSd = false;

    void addEntry(const String& key, const String& values) {
        if (gEntryCount >= MAX_DIALOGUE_ENTRIES) return;
        gEntries[gEntryCount].key = key;
        gEntries[gEntryCount].values = values;
        gEntryCount++;
    }

    bool loadFromSd() {
        if (!PetStorage::isAvailable() || !SD.exists(DIALOGUE_PATH)) return false;
        File file = SD.open(DIALOGUE_PATH, FILE_READ);
        if (!file) return false;

        gEntryCount = 0;
        while (file.available() && gEntryCount < MAX_DIALOGUE_ENTRIES) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() == 0 || line.startsWith("#")) continue;
            int sep = line.indexOf('=');
            if (sep <= 0) continue;
            String key = line.substring(0, sep);
            String values = line.substring(sep + 1);
            key.trim();
            values.trim();
            if (key.length() == 0 || values.length() == 0) continue;
            addEntry(key, values);
        }
        file.close();
        return gEntryCount > 0;
    }

    String findValues(const String& personality, const String& category) {
        String key = personality + "." + category;
        for (int i = 0; i < gEntryCount; i++) {
            if (gEntries[i].key == key) return gEntries[i].values;
        }
        return "";
    }

    String pickFromList(const String& values) {
        if (values.length() == 0) return "";
        int count = 1;
        for (int i = 0; i < values.length(); i++) {
            if (values[i] == '|') count++;
        }
        int target = random(count);
        int current = 0;
        int start = 0;
        for (int i = 0; i <= values.length(); i++) {
            if (i == values.length() || values[i] == '|') {
                if (current == target) {
                    String out = values.substring(start, i);
                    out.trim();
                    return out;
                }
                current++;
                start = i + 1;
            }
        }
        String out = values;
        out.trim();
        return out;
    }
}

void PetDialogue::begin() {
    if (gInitialized) return;
    gInitialized = true;
    gUsingSd = loadFromSd();
    Serial.printf("[DIALOGUE] source=%s entries=%d\n", gUsingSd ? "sd" : "builtin", gEntryCount);
}

bool PetDialogue::usingSdDialogue() {
    begin();
    return gUsingSd;
}

String PetDialogue::pick(const String& personality, const String& category) {
    begin();
    String values = findValues(personality, category);
    return pickFromList(values);
}
