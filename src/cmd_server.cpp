#include "cmd_server.h"

#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

static WiFiServer tcpServer(CmdServer::CMD_PORT);
static WiFiUDP cmdUdp;
static char tcpRequestBuf[4096];
static char tcpResponseBuf[4096];
static char udpRequestBuf[768];
static char udpResponseBuf[1024];

void CmdServer::begin() {
    if (started) return;
    tcpServer.begin();
    tcpServer.setNoDelay(true);
    cmdUdp.begin(CMD_UDP_PORT);
    started = true;
    Serial.printf("[CMD] TCP %d + UDP %d ready\n", CMD_PORT, CMD_UDP_PORT);
}

void CmdServer::tick() {
    if (!started) return;
    tickTCP();
    tickUDP();
}

void CmdServer::tickTCP() {
    WiFiClient client = tcpServer.available();
    if (!client) return;

    int len = 0;
    unsigned long deadline = millis() + 2000;
    while (client.connected() && millis() < deadline) {
        if (!client.available()) {
            delay(1);
            continue;
        }
        char c = client.read();
        if (c == '\n') break;
        if (len < BUF_SIZE - 1) tcpRequestBuf[len++] = c;
    }
    tcpRequestBuf[len] = '\0';

    if (len > 0) {
        Serial.printf("[CMD/TCP] Received: %s\n", tcpRequestBuf);
        processCommand(tcpRequestBuf, tcpResponseBuf, sizeof(tcpResponseBuf));
        client.print(tcpResponseBuf);
        client.print('\n');
    }

    client.stop();
}

void CmdServer::tickUDP() {
    int packetSize = cmdUdp.parsePacket();
    if (packetSize <= 0) return;

    int len = cmdUdp.read(udpRequestBuf, sizeof(udpRequestBuf) - 1);
    if (len <= 0) return;
    udpRequestBuf[len] = '\0';

    while (len > 0 && (udpRequestBuf[len - 1] == '\n' || udpRequestBuf[len - 1] == '\r'))
        udpRequestBuf[--len] = '\0';
    if (len == 0) return;

    Serial.printf("[CMD/UDP] Received: %s\n", udpRequestBuf);
    processCommand(udpRequestBuf, udpResponseBuf, sizeof(udpResponseBuf));

    cmdUdp.beginPacket(cmdUdp.remoteIP(), cmdUdp.remotePort());
    cmdUdp.write((const uint8_t*)udpResponseBuf, strlen(udpResponseBuf));
    cmdUdp.endPacket();
}

void CmdServer::processCommand(const char* json, char* responseBuf, int responseBufSize) {
    snprintf(responseBuf, responseBufSize, "{\"ok\":true}");

    const char* cmdKey = strstr(json, "\"cmd\":\"");
    if (!cmdKey) {
        snprintf(responseBuf, responseBufSize, "{\"ok\":false,\"error\":\"no cmd\"}");
        return;
    }
    const char* cmdStart = cmdKey + 7;
    const char* cmdEnd = strchr(cmdStart, '"');
    if (!cmdEnd) return;

    int cmdLen = cmdEnd - cmdStart;
    char cmd[20];
    if (cmdLen >= (int)sizeof(cmd)) cmdLen = sizeof(cmd) - 1;
    memcpy(cmd, cmdStart, cmdLen);
    cmd[cmdLen] = '\0';

    if (strcmp(cmd, "animate") == 0) {
        const char* stateKey = strstr(json, "\"state\":\"");
        if (stateKey && animateCb) {
            const char* stStart = stateKey + 9;
            const char* stEnd = strchr(stStart, '"');
            if (stEnd) {
                char state[16];
                int stLen = stEnd - stStart;
                if (stLen >= (int)sizeof(state)) stLen = sizeof(state) - 1;
                memcpy(state, stStart, stLen);
                state[stLen] = '\0';
                animateCb(state);
            }
        }
        return;
    }

    if (strcmp(cmd, "text") == 0 || strcmp(cmd, "say") == 0) {
        bool autoSend = (strcmp(cmd, "say") == 0);
        const char* msgKey = strstr(json, "\"msg\":\"");
        if (msgKey && textCb) {
            const char* msgStart = msgKey + 7;
            const char* p = msgStart;
            char msgBuf[256];
            int mi = 0;
            while (*p && *p != '"' && mi < (int)sizeof(msgBuf) - 1) {
                if (*p == '\\' && p[1]) {
                    switch (p[1]) {
                        case '"': msgBuf[mi++] = '"'; p += 2; break;
                        case '\\': msgBuf[mi++] = '\\'; p += 2; break;
                        case 'n': msgBuf[mi++] = '\n'; p += 2; break;
                        default: p += 2; break;
                    }
                } else {
                    msgBuf[mi++] = *p++;
                }
            }
            msgBuf[mi] = '\0';
            textCb(msgBuf, autoSend);
        }
        return;
    }

    if (strcmp(cmd, "notify") == 0) {
        if (notifyCb) {
            JsonDocument doc;
            if (deserializeJson(doc, json) == DeserializationError::Ok) {
                const char* app = doc["app"] | "";
                const char* title = doc["title"] | "";
                const char* body = doc["body"] | "";
                notifyCb(app, title, body);
            }
        }
        return;
    }

    if (strcmp(cmd, "history") == 0) {
        if (historyCb) {
            String history = historyCb();
            strncpy(responseBuf, history.c_str(), responseBufSize - 1);
            responseBuf[responseBufSize - 1] = '\0';
        }
        return;
    }

    if (strcmp(cmd, "sync_enter") == 0) {
        if (!syncEnterCb) {
            snprintf(responseBuf, responseBufSize, "{\"ok\":false,\"error\":\"sync unavailable\"}");
            return;
        }
        String snapshot = syncEnterCb();
        strncpy(responseBuf, snapshot.c_str(), responseBufSize - 1);
        responseBuf[responseBufSize - 1] = '\0';
        return;
    }

    if (strcmp(cmd, "sync_ping") == 0) {
        bool ok = syncPingCb ? syncPingCb() : false;
        snprintf(responseBuf, responseBufSize, ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"sync inactive\"}");
        return;
    }

    if (strcmp(cmd, "sync_leave") == 0) {
        if (!syncLeaveCb) {
            snprintf(responseBuf, responseBufSize, "{\"ok\":false,\"error\":\"sync unavailable\"}");
            return;
        }
        JsonDocument doc;
        if (deserializeJson(doc, json) != DeserializationError::Ok || !doc["snapshot"].is<JsonObject>()) {
            snprintf(responseBuf, responseBufSize, "{\"ok\":false,\"error\":\"bad snapshot\"}");
            return;
        }
        String snapshotJson;
        serializeJson(doc["snapshot"], snapshotJson);
        bool ok = syncLeaveCb(snapshotJson);
        snprintf(responseBuf, responseBufSize, ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"apply failed\"}");
        return;
    }

    snprintf(responseBuf, responseBufSize, "{\"ok\":false,\"error\":\"unknown cmd\"}");
}
