#include "ime_dict.h"

#include <SD.h>

#include "pet_storage.h"

namespace {
struct ImeEntry {
    const char* pinyin;
    const char* candidates[ImeDict::MAX_CANDIDATES];
};

const char* LEGACY_DICT_PATH = "/pet/ime/pinyin.txt";
const char* IME_DIR = "/pet/ime";
bool gInitialized = false;
bool gUseSdDictionary = false;
bool gUseBucketFiles = false;

static const ImeEntry FALLBACK_ENTRIES[] = {
    {"ni", {u8"\u4f60", u8"\u5462", u8"\u6ce5", nullptr, nullptr}},
    {"hao", {u8"\u597d", u8"\u53f7", u8"\u6d69", nullptr, nullptr}},
    {"nihao", {u8"\u4f60\u597d", nullptr, nullptr, nullptr, nullptr}},
    {"wo", {u8"\u6211", u8"\u63e1", nullptr, nullptr, nullptr}},
    {"women", {u8"\u6211\u4eec", nullptr, nullptr, nullptr, nullptr}},
    {"woai", {u8"\u6211\u7231", nullptr, nullptr, nullptr, nullptr}},
    {"woaini", {u8"\u6211\u7231\u4f60", nullptr, nullptr, nullptr, nullptr}},
    {"shi", {u8"\u662f", u8"\u65f6", u8"\u4e8b", nullptr, nullptr}},
    {"de", {u8"\u7684", u8"\u5f97", u8"\u5730", nullptr, nullptr}},
    {"le", {u8"\u4e86", u8"\u4e50", nullptr, nullptr, nullptr}},
    {"ma", {u8"\u5417", u8"\u5988", nullptr, nullptr, nullptr}},
    {"shenme", {u8"\u4ec0\u4e48", nullptr, nullptr, nullptr, nullptr}},
    {"weishenme", {u8"\u4e3a\u4ec0\u4e48", nullptr, nullptr, nullptr, nullptr}},
    {"zenme", {u8"\u600e\u4e48", nullptr, nullptr, nullptr, nullptr}},
    {"xianzai", {u8"\u73b0\u5728", nullptr, nullptr, nullptr, nullptr}},
    {"jintian", {u8"\u4eca\u5929", nullptr, nullptr, nullptr, nullptr}},
    {"mingtian", {u8"\u660e\u5929", nullptr, nullptr, nullptr, nullptr}},
    {"tianqi", {u8"\u5929\u6c14", nullptr, nullptr, nullptr, nullptr}},
    {"xinwen", {u8"\u65b0\u95fb", nullptr, nullptr, nullptr, nullptr}},
    {"zuijin", {u8"\u6700\u8fd1", nullptr, nullptr, nullptr, nullptr}},
    {"shenzhen", {u8"\u6df1\u5733", nullptr, nullptr, nullptr, nullptr}},
    {"wozai", {u8"\u6211\u5728", nullptr, nullptr, nullptr, nullptr}},
    {"zheli", {u8"\u8fd9\u91cc", nullptr, nullptr, nullptr, nullptr}},
    {"huijia", {u8"\u56de\u5bb6", nullptr, nullptr, nullptr, nullptr}},
    {"chumen", {u8"\u51fa\u95e8", nullptr, nullptr, nullptr, nullptr}},
    {"xihuan", {u8"\u559c\u6b22", nullptr, nullptr, nullptr, nullptr}},
    {"maomi", {u8"\u732b\u54aa", nullptr, nullptr, nullptr, nullptr}},
    {"juzi", {u8"\u6a58\u5b50", nullptr, nullptr, nullptr, nullptr}},
    {"jumao", {u8"\u6a58\u732b", nullptr, nullptr, nullptr, nullptr}},
    {"wan", {u8"\u73a9", u8"\u665a", nullptr, nullptr, nullptr}},
    {"wanle", {u8"\u73a9\u4e50", nullptr, nullptr, nullptr, nullptr}},
    {"chi", {u8"\u5403", u8"\u8fdf", nullptr, nullptr, nullptr}},
    {"shui", {u8"\u7761", u8"\u6c34", nullptr, nullptr, nullptr}},
    {"qingli", {u8"\u6e05\u7406", nullptr, nullptr, nullptr, nullptr}},
    {"xiexie", {u8"\u8c22\u8c22", nullptr, nullptr, nullptr, nullptr}},
    {"zaijian", {u8"\u518d\u89c1", nullptr, nullptr, nullptr, nullptr}},
    {"keyi", {u8"\u53ef\u4ee5", nullptr, nullptr, nullptr, nullptr}},
    {"bukeyi", {u8"\u4e0d\u53ef\u4ee5", nullptr, nullptr, nullptr, nullptr}},
    {"duibuqi", {u8"\u5bf9\u4e0d\u8d77", nullptr, nullptr, nullptr, nullptr}},
};

bool candidateExists(const String& needle, String outCandidates[], int candidateCount) {
    for (int i = 0; i < candidateCount; i++) {
        if (outCandidates[i] == needle) return true;
    }
    return false;
}

void addCandidate(const String& value, String outCandidates[], int& candidateCount, int maxCount) {
    if (value.length() == 0 || candidateCount >= maxCount) return;
    if (candidateExists(value, outCandidates, candidateCount)) return;
    outCandidates[candidateCount++] = value;
}

int parseCandidateList(const String& rawList, String outCandidates[], int candidateCount, int maxCount) {
    int start = 0;
    while (start < rawList.length() && candidateCount < maxCount) {
        int comma = rawList.indexOf(',', start);
        String value = (comma >= 0) ? rawList.substring(start, comma) : rawList.substring(start);
        value.trim();
        addCandidate(value, outCandidates, candidateCount, maxCount);
        if (comma < 0) break;
        start = comma + 1;
    }
    return candidateCount;
}

String bucketPathFor(const String& pinyin) {
    if (pinyin.length() == 0) return String(IME_DIR) + "/misc.txt";
    char first = (char)tolower((unsigned char)pinyin[0]);
    if (first >= 'a' && first <= 'z') {
        String path = String(IME_DIR) + "/";
        path += first;
        path += ".txt";
        return path;
    }
    return String(IME_DIR) + "/misc.txt";
}

int lookupFromFile(const String& filePath, const String& pinyin, String outCandidates[], int maxCount) {
    File file = SD.open(filePath.c_str(), FILE_READ);
    if (!file) return 0;

    int candidateCount = 0;
    bool exactMatched = false;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int sep = line.indexOf('=');
        if (sep <= 0) continue;

        String key = line.substring(0, sep);
        String values = line.substring(sep + 1);
        key.trim();
        values.trim();

        if (key.equalsIgnoreCase(pinyin)) {
            candidateCount = parseCandidateList(values, outCandidates, 0, maxCount);
            exactMatched = candidateCount > 0;
            break;
        }
    }

    file.close();
    if (exactMatched) return candidateCount;

    file = SD.open(filePath.c_str(), FILE_READ);
    if (!file) return 0;

    while (file.available() && candidateCount < maxCount) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int sep = line.indexOf('=');
        if (sep <= 0) continue;

        String key = line.substring(0, sep);
        String values = line.substring(sep + 1);
        key.trim();
        values.trim();

        if (key.startsWith(pinyin)) {
            candidateCount = parseCandidateList(values, outCandidates, candidateCount, maxCount);
        }
    }

    file.close();
    return candidateCount;
}

int lookupFromSd(const String& pinyin, String outCandidates[], int maxCount) {
    if (gUseBucketFiles) {
        return lookupFromFile(bucketPathFor(pinyin), pinyin, outCandidates, maxCount);
    }
    return lookupFromFile(LEGACY_DICT_PATH, pinyin, outCandidates, maxCount);
}

int lookupFallback(const String& pinyin, String outCandidates[], int maxCount) {
    int candidateCount = 0;

    for (const auto& entry : FALLBACK_ENTRIES) {
        if (pinyin.equalsIgnoreCase(entry.pinyin)) {
            for (int i = 0; i < ImeDict::MAX_CANDIDATES && candidateCount < maxCount; i++) {
                if (!entry.candidates[i]) break;
                addCandidate(String(entry.candidates[i]), outCandidates, candidateCount, maxCount);
            }
            return candidateCount;
        }
    }

    for (const auto& entry : FALLBACK_ENTRIES) {
        if (String(entry.pinyin).startsWith(pinyin)) {
            for (int i = 0; i < ImeDict::MAX_CANDIDATES && candidateCount < maxCount; i++) {
                if (!entry.candidates[i]) break;
                addCandidate(String(entry.candidates[i]), outCandidates, candidateCount, maxCount);
            }
        }
    }

    return candidateCount;
}
}

void ImeDict::begin() {
    if (gInitialized) return;
    gInitialized = true;
    gUseBucketFiles = PetStorage::isAvailable() && SD.exists("/pet/ime/a.txt");
    gUseSdDictionary = PetStorage::isAvailable() && (gUseBucketFiles || SD.exists(LEGACY_DICT_PATH));
    Serial.printf("[IME] dictionary=%s mode=%s\n",
                  gUseSdDictionary ? "sd" : "fallback",
                  gUseBucketFiles ? "bucket" : "single");
}

bool ImeDict::usingSdDictionary() {
    begin();
    return gUseSdDictionary;
}

int ImeDict::lookup(const String& pinyin, String outCandidates[], int maxCount) {
    begin();
    for (int i = 0; i < maxCount; i++) {
        outCandidates[i] = "";
    }
    if (pinyin.length() == 0) return 0;

    if (gUseSdDictionary) {
        int count = lookupFromSd(pinyin, outCandidates, maxCount);
        if (count > 0) return count;
    }

    return lookupFallback(pinyin, outCandidates, maxCount);
}
