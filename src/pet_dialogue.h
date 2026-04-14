#pragma once

#include <Arduino.h>

namespace PetDialogue {
    void begin();
    bool usingSdDialogue();
    String pick(const String& personality, const String& category);
    String value(const String& key);
    String option(const String& key, int index);
}
