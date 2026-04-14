#include "state_broadcast.h"
#include <WiFiUdp.h>
#include "utils.h"

static WiFiUDP udp;
static Timer broadcastTimer{200}; // 5Hz

static constexpr uint16_t BROADCAST_PORT = 19820;
static char unicastHost[64] = {0};

// Helper: send a UDP packet to both broadcast and unicast targets (for idempotent state sync)
static void sendPacket(const char* buf, int len) {
    udp.beginPacket("255.255.255.255", BROADCAST_PORT);
    udp.write((const uint8_t*)buf, len);
    udp.endPacket();

    if (unicastHost[0]) {
        udp.beginPacket(unicastHost, BROADCAST_PORT);
        udp.write((const uint8_t*)buf, len);
        udp.endPacket();
    }
}

// Helper: send a UDP packet once (for one-shot messages like chat/pixelart)
// Sends via both broadcast and unicast — iPhone hotspot blocks broadcast
static void sendPacketOnce(const char* buf, int len) {
    udp.beginPacket("255.255.255.255", BROADCAST_PORT);
    udp.write((const uint8_t*)buf, len);
    udp.endPacket();

    if (unicastHost[0]) {
        udp.beginPacket(unicastHost, BROADCAST_PORT);
        udp.write((const uint8_t*)buf, len);
        udp.endPacket();
    }
}

void stateBroadcastBegin(const char* target) {
    udp.begin(BROADCAST_PORT);
    if (target && strlen(target) > 0) {
        strncpy(unicastHost, target, sizeof(unicastHost) - 1);
        unicastHost[sizeof(unicastHost) - 1] = '\0';
    }
}

void stateBroadcastTick(int state, int frame, const char* mode,
                        float normX, float normY, int direction,
                        int weatherType, float temperature,
                        int moisture, int humidity,
                        const char* deviceId, const char* deviceType,
                        int capTouch, int capKeyboard, int capMic, int capSpeaker) {
    if (!broadcastTimer.tick()) return;

    char buf[256];
    int len;
    const char* did = (deviceId && deviceId[0]) ? deviceId : "unknown";
    const char* dt = (deviceType && deviceType[0]) ? deviceType : "unknown";
    int touch = capTouch < 0 ? 0 : capTouch;
    int keyboard = capKeyboard < 0 ? 0 : capKeyboard;
    int mic = capMic < 0 ? 0 : capMic;
    int speaker = capSpeaker < 0 ? 0 : capSpeaker;
    if (temperature > -999) {
        len = snprintf(buf, sizeof(buf),
            "{\"s\":%d,\"f\":%d,\"m\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"d\":%d,\"w\":%d,\"t\":%.1f,\"h\":%d,\"rh\":%d,\"did\":\"%s\",\"dt\":\"%s\",\"tc\":%d,\"kc\":%d,\"mc\":%d,\"sp\":%d}",
            state, frame, mode, normX, normY, direction, weatherType, temperature, moisture, humidity,
            did, dt, touch, keyboard, mic, speaker);
    } else {
        len = snprintf(buf, sizeof(buf),
            "{\"s\":%d,\"f\":%d,\"m\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"d\":%d,\"w\":%d,\"h\":%d,\"rh\":%d,\"did\":\"%s\",\"dt\":\"%s\",\"tc\":%d,\"kc\":%d,\"mc\":%d,\"sp\":%d}",
            state, frame, mode, normX, normY, direction, weatherType, moisture, humidity,
            did, dt, touch, keyboard, mic, speaker);
    }

    if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
    sendPacket(buf, len);
}

void stateBroadcastPixelArt(int size, const char* rows[], int rowCount) {
    // Build JSON: {"type":"pixelart","size":8,"rows":["00330000",...]}
    char buf[512];
    const int bufMax = (int)sizeof(buf) - 4;  // Reserve space for closing "]}"\0
    int pos = snprintf(buf, sizeof(buf), "{\"type\":\"pixelart\",\"size\":%d,\"rows\":[", size);
    if (pos >= bufMax) return;

    for (int i = 0; i < rowCount && i < size; i++) {
        int rowLen = strlen(rows[i]);
        // Need: comma(1) + quote(1) + rowLen + quote(1) = rowLen+3
        if (pos + rowLen + 3 >= bufMax) break;
        if (i > 0) buf[pos++] = ',';
        buf[pos++] = '"';
        memcpy(buf + pos, rows[i], rowLen);
        pos += rowLen;
        buf[pos++] = '"';
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");

    sendPacketOnce(buf, pos);
}

void stateBroadcastChatMsg(const char* role, const char* text) {
    // Build JSON: {"type":"chat","role":"user","text":"..."}
    // Escape text for JSON safety
    char buf[512];
    int pos = snprintf(buf, sizeof(buf), "{\"type\":\"chat\",\"role\":\"%s\",\"text\":\"", role);

    const char* p = text;
    while (*p && pos < (int)sizeof(buf) - 10) {
        if (*p == '"') { buf[pos++] = '\\'; buf[pos++] = '"'; }
        else if (*p == '\\') { buf[pos++] = '\\'; buf[pos++] = '\\'; }
        else if (*p == '\n') { buf[pos++] = '\\'; buf[pos++] = 'n'; }
        else if ((uint8_t)*p >= 0x20) { buf[pos++] = *p; }
        p++;
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "\"}");

    sendPacketOnce(buf, pos);
}
