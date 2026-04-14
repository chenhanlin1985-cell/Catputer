#include "tts_playback.h"
#include "config.h"
#include "local_tts.h"
#include <WiFiClient.h>
#include <limits.h>

#if defined(CATPUTER_TOUCH_UI)
#define M5DEVICE M5
#else
#define M5DEVICE M5Cardputer
#endif

static void applyPCMSoftGain(int16_t* data, size_t samples, float gain) {
    if (!data || samples == 0 || gain <= 1.0f) return;
    for (size_t i = 0; i < samples; ++i) {
        int32_t boosted = (int32_t)(data[i] * gain);
        if (boosted > INT16_MAX) boosted = INT16_MAX;
        if (boosted < INT16_MIN) boosted = INT16_MIN;
        data[i] = (int16_t)boosted;
    }
}

void TTSPlayback::begin(const String& host, const String& port,
                        int16_t* sharedBuf, size_t maxSamp) {
    ttsHost = host;
    ttsPort = port;
    buffer = sharedBuf;
    maxSamples = maxSamp;
    Serial.printf("[TTS] Initialized, host=%s:%s, buffer=%p, maxSamples=%zu\n",
                  host.c_str(), port.c_str(), sharedBuf, maxSamp);
}

void TTSPlayback::setBuffer(int16_t* sharedBuf, size_t maxSamp) {
    buffer = sharedBuf;
    maxSamples = maxSamp;
    Serial.printf("[TTS] Buffer updated: %p, maxSamples=%zu\n", sharedBuf, maxSamp);
}

void TTSPlayback::attachLocalTTS(LocalTTS* localEngine) {
    local = localEngine;
}

bool TTSPlayback::requestAndPlay(const char* text) {
    if (!buffer || !text || text[0] == '\0') return false;

    size_t samples = 0;
    lastSampleRate = SAMPLE_RATE;

    if (local) {
        size_t localSamples = 0;
        if (local->synthesizeToPCM(text, buffer, maxSamples, localSamples)) {
            samples = localSamples;
            lastSampleRate = local->getSampleRate();
            applyPCMSoftGain(buffer, samples, 1.35f);
            Serial.printf("[TTS] Using local TTS @ %lu Hz\n", (unsigned long)lastSampleRate);
        } else {
            Serial.printf("[TTS] Local TTS unavailable: %s\n", local->availabilityReason());
        }
    }

    if (samples == 0) {
        Serial.println("[TTS] No local PCM data received");
        return false;
    }

    Serial.printf("[TTS] Playing %zu samples (%.1fs) @ %lu Hz\n",
                  samples, (float)samples / lastSampleRate, (unsigned long)lastSampleRate);
    M5DEVICE.Speaker.playRaw(buffer, samples, lastSampleRate, false /* not stereo */, 1, 1, false);
    return true;
}

bool TTSPlayback::isPlaying() const {
    // Guard: treat as "still playing" for 50ms after stop to let DMA drain
    if (stopTime > 0 && millis() - stopTime < STOP_COOLDOWN_MS) return true;
    return M5DEVICE.Speaker.isPlaying();
}

void TTSPlayback::stop() {
    M5DEVICE.Speaker.stop();
    stopTime = millis();
}

size_t TTSPlayback::downloadPCM(const char* text) {
    WiFiClient client;
    client.setTimeout(15000);  // milliseconds
    lastSampleRate = SAMPLE_RATE;

    int port = atoi(ttsPort.c_str());
    if (port <= 0 || port > 65535) {
        Serial.println("[TTS] Invalid port");
        return 0;
    }

    Serial.printf("[TTS] Connecting to %s:%d...\n", ttsHost.c_str(), port);
    if (!client.connect(ttsHost.c_str(), port)) {
        Serial.println("[TTS] Connection failed");
        return 0;
    }

    // Build JSON body on stack — text is short (AI responses ≤ 300 chars)
    // {"text":"...","voice":"zh-CN-XiaoxiaoNeural"}
    char jsonBuf[512];
    // Escape text for JSON string (must handle newlines, quotes, backslashes)
    char escaped[384];
    int ei = 0;
    for (const char* p = text; *p && ei < (int)sizeof(escaped) - 3; p++) {
        if (*p == '"')       { escaped[ei++] = '\\'; escaped[ei++] = '"'; }
        else if (*p == '\\') { escaped[ei++] = '\\'; escaped[ei++] = '\\'; }
        else if (*p == '\n') { escaped[ei++] = '\\'; escaped[ei++] = 'n'; }
        else if (*p == '\r') { escaped[ei++] = '\\'; escaped[ei++] = 'r'; }
        else if (*p == '\t') { escaped[ei++] = '\\'; escaped[ei++] = 't'; }
        else if ((uint8_t)*p < 0x20) { continue; }  // skip other control chars
        else { escaped[ei++] = *p; }
    }
    escaped[ei] = '\0';

    int bodyLen = snprintf(jsonBuf, sizeof(jsonBuf),
        "{\"text\":\"%s\",\"voice\":\"zh-CN-XiaoxiaoNeural\"}", escaped);
    if (bodyLen >= (int)sizeof(jsonBuf)) bodyLen = sizeof(jsonBuf) - 1;  // cap to actual written

    // Send HTTP request
    client.printf("POST /v1/audio/speech HTTP/1.1\r\n"
                  "Host: %s:%s\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: %d\r\n"
                  "Connection: close\r\n\r\n",
                  ttsHost.c_str(), ttsPort.c_str(), bodyLen);
    client.write((const uint8_t*)jsonBuf, bodyLen);

    Serial.printf("[TTS] Request sent, %d bytes body\n", bodyLen);

    // Read HTTP response headers
    unsigned long deadline = millis() + 30000;
    bool httpOk = false;
    int httpStatus = 0;
    int contentLength = -1;
    char hdrBuf[256];
    int hdrLen = 0;

    while (client.connected() && millis() < deadline) {
        if (!client.available()) { delay(10); continue; }
        char c = client.read();
        if (c == '\n') {
            if (hdrLen > 0 && hdrBuf[hdrLen - 1] == '\r') hdrLen--;
            hdrBuf[hdrLen] = '\0';
            if (hdrLen == 0) break;  // end of headers
            if (strstr(hdrBuf, "HTTP/") == hdrBuf) {
                Serial.printf("[TTS] Status: %s\n", hdrBuf);
                const char* statusPtr = strchr(hdrBuf, ' ');
                if (statusPtr) httpStatus = atoi(statusPtr + 1);
                if (httpStatus == 200) httpOk = true;
            }
            // Parse Content-Length
            if (strncasecmp(hdrBuf, "Content-Length:", 15) == 0) {
                contentLength = atoi(hdrBuf + 15);
            }
            hdrLen = 0;
        } else if (hdrLen < (int)sizeof(hdrBuf) - 1) {
            hdrBuf[hdrLen++] = c;
        }
    }

    if (!httpOk) {
        char errBody[256];
        int errLen = 0;
        unsigned long errDeadline = millis() + 2000;
        while ((client.connected() || client.available()) && millis() < errDeadline && errLen < (int)sizeof(errBody) - 1) {
            if (!client.available()) { delay(5); continue; }
            errBody[errLen++] = client.read();
        }
        errBody[errLen] = '\0';
        if (errLen > 0) {
            Serial.printf("[TTS] Error body: %s\n", errBody);
        }
        Serial.println("[TTS] HTTP error from proxy");
        client.stop();
        return 0;
    }

    // Read PCM body into shared buffer
    size_t maxBytes = maxSamples * sizeof(int16_t);  // 96KB
    size_t bytesRead = 0;
    uint8_t* dst = (uint8_t*)buffer;

    // If content-length known, cap it
    size_t toRead = maxBytes;
    if (contentLength > 0 && (size_t)contentLength < toRead) {
        toRead = (size_t)contentLength;
    }

    while (bytesRead < toRead && (client.connected() || client.available()) && millis() < deadline) {
        if (!client.available()) { delay(1); continue; }
        int avail = client.available();
        int chunk = avail;
        if (bytesRead + chunk > toRead) chunk = toRead - bytesRead;
        int got = client.read(dst + bytesRead, chunk);
        if (got > 0) bytesRead += got;
    }

    client.stop();

    size_t samplesRead = bytesRead / sizeof(int16_t);
    Serial.printf("[TTS] Downloaded %zu bytes = %zu samples (%.1fs), heap=%u\n",
                  bytesRead, samplesRead, (float)samplesRead / SAMPLE_RATE, ESP.getFreeHeap());

    return samplesRead;
}

void TTSPlayback::drawSpeakingBar(M5Canvas& canvas) {
    int barY = SCREEN_H - INPUT_BAR_H;
    uint16_t speakColor = rgb565(40, 80, 50);  // Dark green
    canvas.fillRect(0, barY, SCREEN_W, INPUT_BAR_H, speakColor);
    canvas.drawFastHLine(0, barY, SCREEN_W, rgb565(60, 160, 80));

    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextColor(Color::CHAT_AI);
    canvas.setTextSize(1);

    static const char* msgs[] = {
        "Speaking", "Speaking.", "Speaking..", "Speaking..."
    };
    canvas.drawString(msgs[(millis() / 400) % 4], 4, barY + 4);
}
