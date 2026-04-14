#include "bluetooth_keyboard_mode.h"

#include <cstring>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#include <esp_heap_caps.h>

namespace {
struct KeyReport {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
};

static const uint8_t kHidReportDescriptor[] = {
    USAGE_PAGE(1),      0x01,
    USAGE(1),           0x06,
    COLLECTION(1),      0x01,
    REPORT_ID(1),       0x01,
    USAGE_PAGE(1),      0x07,
    USAGE_MINIMUM(1),   0xE0,
    USAGE_MAXIMUM(1),   0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1),     0x01,
    REPORT_COUNT(1),    0x08,
    HIDINPUT(1),        0x02,
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x08,
    HIDINPUT(1),        0x01,
    REPORT_COUNT(1),    0x05,
    REPORT_SIZE(1),     0x01,
    USAGE_PAGE(1),      0x08,
    USAGE_MINIMUM(1),   0x01,
    USAGE_MAXIMUM(1),   0x05,
    HIDOUTPUT(1),       0x02,
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x03,
    HIDOUTPUT(1),       0x01,
    REPORT_COUNT(1),    0x06,
    REPORT_SIZE(1),     0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65,
    USAGE_PAGE(1),      0x07,
    USAGE_MINIMUM(1),   0x00,
    USAGE_MAXIMUM(1),   0x65,
    HIDINPUT(1),        0x00,
    END_COLLECTION(0)
};

static bool bleInitialized = false;
static BLEServer* bleServer = nullptr;
static BLEHIDDevice* bleHid = nullptr;
static BLECharacteristic* bleInputKeyboard = nullptr;
static BLEAdvertising* bleAdvertising = nullptr;
static bool bleAdvertisingStarted = false;
static bool bleKeyboardActive = false;

class KeyboardServerCallbacks : public BLEServerCallbacks {
public:
    bool connected = false;
    uint16_t connId = 0xFFFF;

    void onConnect(BLEServer* pServer) override {
        connected = true;
        bleAdvertisingStarted = false;
    }

    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t* param) override {
        connected = true;
        connId = param ? param->connect.conn_id : 0xFFFF;
        bleAdvertisingStarted = false;
    }

    void onDisconnect(BLEServer* pServer) override {
        connected = false;
        connId = 0xFFFF;
        pServer->startAdvertising();
        bleAdvertisingStarted = true;
    }

    void onDisconnect(BLEServer* pServer, esp_ble_gatts_cb_param_t* param) override {
        connected = false;
        connId = 0xFFFF;
        pServer->startAdvertising();
        bleAdvertisingStarted = true;
    }
};

static KeyboardServerCallbacks bleCallbacks;
}

bool BluetoothKeyboardMode::isConnected() const {
    return bleInitialized && bleCallbacks.connected;
}

void BluetoothKeyboardMode::resetState() {
    prevExitCombo = false;
    ignoreExitUntil = 0;
    memset(&prevScanReport, 0, sizeof(prevScanReport));
    memset(reportQueue, 0, sizeof(reportQueue));
    reportQueueHead = 0;
    reportQueueTail = 0;
    reportQueueCount = 0;
    lastReportSentAt = 0;
    lastDebugAt = 0;
    statsEnqueued = 0;
    statsSent = 0;
    statsDropped = 0;
    statsSaturated = 0;
    lastRenderedConnected = false;
    lastRenderAt = 0;
}

bool BluetoothKeyboardMode::enqueueReport(const HidKeyReport& report) {
    if (reportQueueCount >= REPORT_QUEUE_SIZE) {
        statsSaturated++;
        HidKeyReport empty = {};
        if (memcmp(&report, &empty, sizeof(HidKeyReport)) == 0) {
            // Keep release event highest priority to avoid stuck keys.
            reportQueueHead = 0;
            reportQueueTail = 0;
            reportQueueCount = 0;
            reportQueue[0] = empty;
            reportQueueHead = 0;
            reportQueueTail = 1;
            reportQueueCount = 1;
            statsEnqueued++;
            return true;
        }
        statsDropped++;
        return false;
    }
    reportQueue[reportQueueTail] = report;
    reportQueueTail = (reportQueueTail + 1) % REPORT_QUEUE_SIZE;
    reportQueueCount++;
    statsEnqueued++;
    return true;
}

void BluetoothKeyboardMode::processQueue() {
    if (!bleKeyboardActive || !bleInputKeyboard || !bleCallbacks.connected) return;
    if (reportQueueCount == 0) return;
    while (reportQueueCount > 0) {
        unsigned long now = millis();
        if (lastReportSentAt != 0 && (now - lastReportSentAt) < REPORT_INTERVAL_MS) {
            break;
        }
        const HidKeyReport& nextReport = reportQueue[reportQueueHead];
        bleInputKeyboard->setValue((uint8_t*)&nextReport, sizeof(nextReport));
        bleInputKeyboard->notify();
        reportQueueHead = (reportQueueHead + 1) % REPORT_QUEUE_SIZE;
        reportQueueCount--;
        lastReportSentAt = now;
        statsSent++;
    }
}

void BluetoothKeyboardMode::begin() {
    if (!bleInitialized) {
        Serial.println("[BLE] Initializing BLE HID keyboard");
        BLEDevice::init(advertisedName());
        bleServer = BLEDevice::createServer();
        bleServer->setCallbacks(&bleCallbacks);

        bleHid = new BLEHIDDevice(bleServer);
        bleInputKeyboard = bleHid->inputReport(0x01);
        bleHid->manufacturer()->setValue("Catputer");
        bleHid->pnp(0x02, 0x05ac, 0x820a, 0x0210);
        bleHid->hidInfo(0x00, 0x01);

        BLESecurity* security = new BLESecurity();
        security->setAuthenticationMode(ESP_LE_AUTH_BOND);
        security->setCapability(ESP_IO_CAP_NONE);
        security->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
        security->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

        bleHid->reportMap((uint8_t*)kHidReportDescriptor, sizeof(kHidReportDescriptor));
        bleHid->startServices();
        bleAdvertising = bleServer->getAdvertising();
        bleAdvertising->setAppearance(HID_KEYBOARD);
        bleAdvertising->addServiceUUID(bleHid->hidService()->getUUID());
        bleAdvertising->setScanResponse(false);
        bleAdvertising->start();
        bleAdvertisingStarted = true;
        bleHid->setBatteryLevel(100);
        bleInitialized = true;
    } else if (!bleCallbacks.connected && bleAdvertising) {
        Serial.println("[BLE] Restart advertising");
        bleAdvertising->start();
        bleAdvertisingStarted = true;
    }

    bleKeyboardActive = true;
    started = true;
    resetState();
    // Prevent the same Fn+B keypress that entered this mode from immediately
    // triggering the exit combo on the next frame.
    prevExitCombo = true;
    ignoreExitUntil = millis() + 600;
}

void BluetoothKeyboardMode::end() {
    if (!started) return;
    Serial.println("[BLE] Leaving BLE HID keyboard mode");
    if (bleInputKeyboard) {
        KeyReport empty = {};
        bleInputKeyboard->setValue((uint8_t*)&empty, sizeof(empty));
        bleInputKeyboard->notify();
    }
    if (bleCallbacks.connected && bleServer && bleCallbacks.connId != 0xFFFF) {
        bleServer->disconnect(bleCallbacks.connId);
    } else if (bleAdvertising && !bleAdvertisingStarted) {
        bleAdvertising->start();
        bleAdvertisingStarted = true;
    }
    bleKeyboardActive = false;
    started = false;
    resetState();
}

bool BluetoothKeyboardMode::shouldExit(const Keyboard_Class::KeysState& ks) {
    (void)ks;
    prevExitCombo = false;
    return false;
}

void BluetoothKeyboardMode::handleKeys(const Keyboard_Class::KeysState& ks, bool keyPressed) {
    (void)keyPressed;
    if (!bleKeyboardActive || !bleInputKeyboard || !bleCallbacks.connected) {
        memset(&prevScanReport, 0, sizeof(prevScanReport));
        reportQueueHead = 0;
        reportQueueTail = 0;
        reportQueueCount = 0;
        lastReportSentAt = 0;
        return;
    }

    HidKeyReport current = {};
    current.modifiers = ks.modifiers;
    size_t currentCount = 0;
    for (uint8_t key : ks.hid_keys) {
        if (key == 0) continue;
        if (currentCount >= 6) break;
        current.keys[currentCount++] = key & 0x7F;
    }

    if (memcmp(&current, &prevScanReport, sizeof(current)) == 0) {
        processQueue();
        return;
    }
    enqueueReport(current);
    prevScanReport = current;
    processQueue();
}

void BluetoothKeyboardMode::update(M5Canvas& canvas) {
    processQueue();
    if (bleCallbacks.connected) {
        unsigned long now = millis();
        if (lastDebugAt == 0 || now - lastDebugAt >= 1000) {
            Serial.printf(
                "[BLEDBG] q=%u enq=%lu sent=%lu drop=%lu sat=%lu heap=%u largest=%u\n",
                static_cast<unsigned>(reportQueueCount),
                static_cast<unsigned long>(statsEnqueued),
                static_cast<unsigned long>(statsSent),
                static_cast<unsigned long>(statsDropped),
                static_cast<unsigned long>(statsSaturated),
                static_cast<unsigned>(ESP.getFreeHeap()),
                static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
            lastDebugAt = now;
        }
    }

    const uint16_t bg = rgb565(18, 22, 28);
    bool connected = isConnected();
    unsigned long now = millis();
    bool shouldRedraw = (lastRenderAt == 0) ||
                        (connected != lastRenderedConnected) ||
                        (now - lastRenderAt >= 250);

    if (!shouldRedraw) return;
    lastRenderAt = now;
    lastRenderedConnected = connected;

    canvas.fillSprite(bg);
    canvas.setTextColor(Color::WHITE, bg);
    canvas.setFont(&fonts::efontCN_12);
    canvas.drawString(u8"\u84dd\u7259\u952e\u76d8\u6a21\u5f0f", 12, 12);

    canvas.setTextColor(connected ? rgb565(120, 220, 140) : rgb565(255, 210, 120), bg);
    canvas.drawString(
        connected ? u8"\u5df2\u8fde\u63a5\uff0c\u53ef\u76f4\u63a5\u8f93\u5165"
                  : u8"\u7b49\u5f85\u914d\u5bf9\u8fde\u63a5",
        12, 34);

    canvas.setTextColor(Color::WHITE, bg);
    canvas.drawString(u8"\u8bbe\u5907\u540d: Catputer KB", 12, 56);
    canvas.drawString(u8"\u84dd\u7259\u6a21\u5f0f\u4e3a\u72ec\u7acb\u6a21\u5f0f", 12, 78);
    canvas.drawString(u8"\u652f\u6301: \u5b57\u6bcd\u6570\u5b57 / \u7a7a\u683c / \u56de\u8f66 / \u9000\u683c / Tab", 12, 100);
    canvas.drawString(u8"\u4f7f\u7528\u5b8c\u8bf7\u91cd\u542f\u8fd4\u56de\u5ba0\u7269\u6a21\u5f0f", 12, 118);
}
