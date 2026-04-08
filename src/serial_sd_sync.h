#pragma once

#include <Arduino.h>
#include <FS.h>

class SerialSDSync {
public:
    void begin();
    void tick();

private:
    static constexpr size_t LINE_BUF_SIZE = 640;

    void processLine(char* line);
    void replyOk(const char* extra = nullptr);
    void replyError(const char* message);
    bool ensureParentDirs(const char* path);
    bool decodeHexChunk(const char* hex, uint8_t* outBuf, size_t outBufSize, size_t& outLen);
    void closeUpload();

    bool started = false;
    char lineBuf[LINE_BUF_SIZE] = {0};
    size_t lineLen = 0;

    File uploadFile;
    char uploadPath[128] = {0};
    size_t uploadExpected = 0;
    size_t uploadWritten = 0;
};
