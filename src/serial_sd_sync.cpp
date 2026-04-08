#include "serial_sd_sync.h"

#include <ArduinoJson.h>
#include <SD.h>

namespace {
bool startsWithSlash(const char* path) {
    return path && path[0] == '/';
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}
}

void SerialSDSync::begin() {
    if (started) return;
    started = true;
    Serial.println("[SDSYNC] Ready");
}

void SerialSDSync::tick() {
    if (!started) return;

    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            lineBuf[lineLen] = '\0';
            if (lineLen > 0) {
                processLine(lineBuf);
            }
            lineLen = 0;
            lineBuf[0] = '\0';
            continue;
        }

        if (lineLen < LINE_BUF_SIZE - 1) {
            lineBuf[lineLen++] = c;
        } else {
            lineLen = 0;
            lineBuf[0] = '\0';
            replyError("line too long");
        }
    }
}

void SerialSDSync::processLine(char* line) {
    JsonDocument doc;
    auto err = deserializeJson(doc, line);
    if (err != DeserializationError::Ok) {
        replyError("bad json");
        return;
    }

    const char* cmd = doc["cmd"] | "";
    if (strcmp(cmd, "ping") == 0) {
        replyOk("\"pong\":true");
        return;
    }

    if (strcmp(cmd, "sd_mkdir") == 0) {
        const char* path = doc["path"] | "";
        if (!startsWithSlash(path)) {
            replyError("bad path");
            return;
        }
        bool ok = SD.exists(path) || SD.mkdir(path);
        if (!ok) {
            replyError("mkdir failed");
            return;
        }
        replyOk(nullptr);
        return;
    }

    if (strcmp(cmd, "sd_put_begin") == 0) {
        const char* path = doc["path"] | "";
        size_t size = doc["size"] | 0;
        if (!startsWithSlash(path) || size == 0) {
            replyError("bad begin");
            return;
        }

        closeUpload();
        if (!ensureParentDirs(path)) {
            replyError("mkdir parent failed");
            return;
        }
        if (SD.exists(path)) {
            SD.remove(path);
        }

        uploadFile = SD.open(path, FILE_WRITE);
        if (!uploadFile) {
            replyError("open failed");
            return;
        }

        strncpy(uploadPath, path, sizeof(uploadPath) - 1);
        uploadPath[sizeof(uploadPath) - 1] = '\0';
        uploadExpected = size;
        uploadWritten = 0;
        replyOk(nullptr);
        return;
    }

    if (strcmp(cmd, "sd_put_chunk") == 0) {
        const char* data = doc["data"] | "";
        if (!uploadFile || !data[0]) {
            replyError("no upload");
            return;
        }

        uint8_t buf[128];
        size_t outLen = 0;
        if (!decodeHexChunk(data, buf, sizeof(buf), outLen)) {
            replyError("bad chunk");
            return;
        }

        size_t written = uploadFile.write(buf, outLen);
        if (written != outLen) {
            closeUpload();
            replyError("write failed");
            return;
        }
        uploadWritten += written;
        replyOk(nullptr);
        return;
    }

    if (strcmp(cmd, "sd_put_end") == 0) {
        if (!uploadFile) {
            replyError("no upload");
            return;
        }
        uploadFile.flush();
        uploadFile.close();
        bool sizeOk = (uploadWritten == uploadExpected);
        uploadPath[0] = '\0';
        uploadExpected = 0;
        uploadWritten = 0;
        if (!sizeOk) {
            replyError("size mismatch");
            return;
        }
        replyOk(nullptr);
        return;
    }

    if (strcmp(cmd, "sd_list") == 0) {
        const char* path = doc["path"] | "/";
        File dir = SD.open(path);
        if (!dir || !dir.isDirectory()) {
            replyError("list failed");
            return;
        }
        String names = "";
        File entry = dir.openNextFile();
        while (entry) {
            if (names.length() > 0) names += ",";
            names += entry.name();
            entry.close();
            entry = dir.openNextFile();
        }
        dir.close();
        Serial.printf("{\"ok\":true,\"files\":\"%s\"}\n", names.c_str());
        return;
    }

    replyError("unknown cmd");
}

void SerialSDSync::replyOk(const char* extra) {
    if (extra && extra[0]) {
        Serial.printf("{\"ok\":true,%s}\n", extra);
    } else {
        Serial.println("{\"ok\":true}");
    }
}

void SerialSDSync::replyError(const char* message) {
    Serial.printf("{\"ok\":false,\"error\":\"%s\"}\n", message ? message : "error");
}

bool SerialSDSync::ensureParentDirs(const char* path) {
    if (!startsWithSlash(path)) return false;
    char tmp[128];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    for (char* p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (!SD.exists(tmp) && !SD.mkdir(tmp)) {
                return false;
            }
            *p = '/';
        }
    }
    return true;
}

bool SerialSDSync::decodeHexChunk(const char* hex, uint8_t* outBuf, size_t outBufSize, size_t& outLen) {
    outLen = 0;
    size_t hexLen = strlen(hex);
    if ((hexLen % 2) != 0) return false;
    if ((hexLen / 2) > outBufSize) return false;

    for (size_t i = 0; i < hexLen; i += 2) {
        int hi = hexValue(hex[i]);
        int lo = hexValue(hex[i + 1]);
        if (hi < 0 || lo < 0) return false;
        outBuf[outLen++] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

void SerialSDSync::closeUpload() {
    if (uploadFile) {
        uploadFile.close();
    }
    uploadPath[0] = '\0';
    uploadExpected = 0;
    uploadWritten = 0;
}
