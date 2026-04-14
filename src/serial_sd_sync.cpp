#include "serial_sd_sync.h"

#include <ArduinoJson.h>
#include <esp_system.h>
#include "config.h"
#include "pet_storage.h"

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

uint32_t updateCrc32(uint32_t crc, const uint8_t* data, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1)));
        }
    }
    return ~crc;
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

    if (strcmp(cmd, "restart") == 0) {
        replyOk("\"restarting\":true");
        Serial.flush();
        delay(100);
        ESP.restart();
        return;
    }

    if (strcmp(cmd, "get_config") == 0) {
        JsonDocument response;
        response["ok"] = true;
        JsonObject config = response["config"].to<JsonObject>();
        config["wifi_ssid"] = Config::getSSID();
        config["wifi_ssid2"] = Config::getSSID2();
        config["api_host"] = Config::getGatewayHost();
        config["api_port"] = Config::getGatewayPort();
        config["stt_host"] = Config::getSttHost();
        config["stt_port"] = Config::getSttPort();
        config["api_host2"] = Config::getGatewayHost2();
        config["city"] = Config::getCity();
        config["auto_speak"] = Config::getAutoSpeak();
        config["local_tts"] = Config::getPreferLocalTTS();
        config["speaker_volume"] = Config::getSpeakerVolume();
        serializeJson(response, Serial);
        Serial.println();
        return;
    }

    if (strcmp(cmd, "set_config") == 0) {
        if (doc["wifi_ssid"].is<const char*>()) Config::setSSID(doc["wifi_ssid"].as<const char*>());
        if (doc["wifi_pass"].is<const char*>()) Config::setPassword(doc["wifi_pass"].as<const char*>());
        if (doc["wifi_ssid2"].is<const char*>()) Config::setSSID2(doc["wifi_ssid2"].as<const char*>());
        if (doc["wifi_pass2"].is<const char*>()) Config::setPassword2(doc["wifi_pass2"].as<const char*>());
        if (doc["api_key"].is<const char*>()) Config::setApiKey(doc["api_key"].as<const char*>());
        if (doc["api_host"].is<const char*>()) Config::setGatewayHost(doc["api_host"].as<const char*>());
        if (doc["api_port"].is<const char*>()) Config::setGatewayPort(doc["api_port"].as<const char*>());
        if (doc["gateway_token"].is<const char*>()) Config::setGatewayToken(doc["gateway_token"].as<const char*>());
        if (doc["stt_host"].is<const char*>()) Config::setSttHost(doc["stt_host"].as<const char*>());
        if (doc["stt_port"].is<const char*>()) Config::setSttPort(doc["stt_port"].as<const char*>());
        if (doc["api_host2"].is<const char*>()) Config::setGatewayHost2(doc["api_host2"].as<const char*>());
        if (doc["city"].is<const char*>()) Config::setCity(doc["city"].as<const char*>());
        if (doc["auto_speak"].is<bool>()) Config::setAutoSpeak(doc["auto_speak"].as<bool>());
        if (doc["local_tts"].is<bool>()) Config::setPreferLocalTTS(doc["local_tts"].as<bool>());
        if (doc["speaker_volume"].is<uint8_t>()) Config::setSpeakerVolume(doc["speaker_volume"].as<uint8_t>());

        Config::save();
        replyOk("\"saved\":true");
        return;
    }

    if (strcmp(cmd, "sd_mkdir") == 0) {
        const char* path = doc["path"] | "";
        if (!startsWithSlash(path)) {
            replyError("bad path");
            return;
        }
        bool ok = PetStorage::fs().exists(path) || PetStorage::fs().mkdir(path);
        if (!ok) {
            replyError("mkdir failed");
            return;
        }
        replyOk(nullptr);
        return;
    }

    if (strcmp(cmd, "sd_remove") == 0) {
        const char* path = doc["path"] | "";
        if (!startsWithSlash(path)) {
            replyError("bad path");
            return;
        }
        if (PetStorage::fs().exists(path) && !PetStorage::fs().remove(path)) {
            replyError("remove failed");
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
        if (PetStorage::fs().exists(path)) {
            PetStorage::fs().remove(path);
        }

        uploadFile = PetStorage::fs().open(path, FILE_WRITE);
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

        uint8_t buf[512];
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
        File dir = PetStorage::fs().open(path);
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

    if (strcmp(cmd, "sd_crc32") == 0) {
        const char* path = doc["path"] | "";
        if (!startsWithSlash(path)) {
            replyError("bad path");
            return;
        }
        File file = PetStorage::fs().open(path, FILE_READ);
        if (!file || file.isDirectory()) {
            replyError("open failed");
            return;
        }

        uint8_t buf[256];
        uint32_t crc = 0;
        size_t total = 0;
        while (file.available()) {
            size_t n = file.read(buf, sizeof(buf));
            if (n == 0) break;
            crc = updateCrc32(crc, buf, n);
            total += n;
        }
        file.close();
        Serial.printf("{\"ok\":true,\"size\":%u,\"crc32\":\"%08lX\"}\n",
                      static_cast<unsigned>(total), static_cast<unsigned long>(crc));
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
            if (!PetStorage::fs().exists(tmp) && !PetStorage::fs().mkdir(tmp)) {
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
