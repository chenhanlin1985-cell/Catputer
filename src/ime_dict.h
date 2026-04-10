#pragma once

#include <Arduino.h>

namespace ImeDict {
    static constexpr int MAX_CANDIDATES = 30;

    void begin();
    bool usingSdDictionary();
    int lookup(const String& pinyin, String outCandidates[], int maxCount = MAX_CANDIDATES);
}
