#pragma once

#include <M5Cardputer.h>
#include "utils.h"

class BluetoothKeyboardMode {
public:
    void begin();
    void end();
    void update(M5Canvas& canvas);
    void handleKeys(const Keyboard_Class::KeysState& ks, bool keyPressed);
    bool shouldExit(const Keyboard_Class::KeysState& ks);
    bool isConnected() const;
    const char* advertisedName() const { return "Catputer KB"; }

private:
    struct HidKeyReport {
        uint8_t modifiers = 0;
        uint8_t reserved = 0;
        uint8_t keys[6] = {0, 0, 0, 0, 0, 0};
    };

    static constexpr size_t REPORT_QUEUE_SIZE = 24;
    static constexpr unsigned long REPORT_INTERVAL_MS = 8;

    bool started = false;
    bool prevExitCombo = false;
    unsigned long ignoreExitUntil = 0;
    HidKeyReport prevScanReport = {};
    HidKeyReport reportQueue[REPORT_QUEUE_SIZE] = {};
    size_t reportQueueHead = 0;
    size_t reportQueueTail = 0;
    size_t reportQueueCount = 0;
    unsigned long lastReportSentAt = 0;
    unsigned long lastDebugAt = 0;
    uint32_t statsEnqueued = 0;
    uint32_t statsSent = 0;
    uint32_t statsDropped = 0;
    uint32_t statsSaturated = 0;

    void resetState();
    bool enqueueReport(const HidKeyReport& report);
    void processQueue();
    bool lastRenderedConnected = false;
    unsigned long lastRenderAt = 0;
};
