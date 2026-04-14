#pragma once

#include <Arduino.h>

namespace SdOta {
    enum class State : uint8_t {
        Idle,
        Running,
        Success,
        Failed
    };

    struct Snapshot {
        State state = State::Idle;
        size_t totalBytes = 0;
        size_t writtenBytes = 0;
        int progressPercent = 0;
        String message;
        bool rebootSuggested = false;
    };

    void begin();
    bool start(const char* path);
    void tick();
    bool isBusy();
    Snapshot snapshot();
    void reset();
}

