#include "sd_ota.h"

#include "pet_storage.h"
#include <Update.h>
#include <esp_ota_ops.h>

namespace {
    SdOta::Snapshot gSnap;
    File gFile;
    String gPath;
    size_t gChunkSize = 2048;
    uint8_t gBuffer[2048];

    void fail(const String& msg) {
        if (Update.isRunning()) {
            Update.abort();
        }
        if (gFile) gFile.close();
        gSnap.state = SdOta::State::Failed;
        gSnap.message = msg;
        gSnap.rebootSuggested = false;
    }
}

void SdOta::begin() {
    reset();
}

void SdOta::reset() {
    if (gFile) gFile.close();
    if (Update.isRunning()) Update.abort();
    gPath = "";
    gSnap = Snapshot{};
    gSnap.message = "就绪";
}

bool SdOta::start(const char* path) {
    if (gSnap.state == State::Running) return false;
    reset();

    if (!PetStorage::isAvailable()) {
        gSnap.state = State::Failed;
        gSnap.message = "SD卡不可用";
        return false;
    }

    const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
    if (!next) {
        gSnap.state = State::Failed;
        gSnap.message = "当前分区不支持OTA";
        return false;
    }

    const char* requestedPath = (path && path[0]) ? path : "/firmware/update.bin";
    const char* candidates[] = {
        requestedPath,
        "/firmware/update.bin",
        "firmware/update.bin",
        "/update.bin",
        "update.bin"
    };
    String tried;
    for (const char* candidate : candidates) {
        if (!candidate || !candidate[0]) continue;
        String candidatePath(candidate);
        if (tried.indexOf(candidatePath) >= 0) continue;
        if (tried.length() > 0) tried += ", ";
        tried += candidatePath;
        gFile = PetStorage::fs().open(candidatePath, FILE_READ);
        if (gFile) {
            gPath = candidatePath;
            break;
        }
    }
    if (!gFile) {
        gSnap.state = State::Failed;
        gSnap.message = String("未找到固件，已尝试: ") + tried;
        return false;
    }

    gSnap.totalBytes = gFile.size();
    gSnap.writtenBytes = 0;
    gSnap.progressPercent = 0;
    if (gSnap.totalBytes < 1024) {
        fail("固件文件太小");
        return false;
    }

    if (!Update.begin(gSnap.totalBytes, U_FLASH)) {
        fail(String("OTA初始化失败: ") + Update.errorString());
        return false;
    }

    gSnap.state = State::Running;
    gSnap.message = String("刷写中: ") + gPath;
    gSnap.rebootSuggested = false;
    return true;
}

void SdOta::tick() {
    if (gSnap.state != State::Running) return;
    if (!gFile) {
        fail("固件文件句柄丢失");
        return;
    }

    if (gFile.available()) {
        size_t toRead = gChunkSize;
        size_t left = gSnap.totalBytes - gSnap.writtenBytes;
        if (left < toRead) toRead = left;
        size_t n = gFile.read(gBuffer, toRead);
        if (n == 0) {
            fail("读取固件失败");
            return;
        }
        size_t written = Update.write(gBuffer, n);
        if (written != n) {
            fail(String("写入失败: ") + Update.errorString());
            return;
        }
        gSnap.writtenBytes += written;
        if (gSnap.totalBytes > 0) {
            gSnap.progressPercent = static_cast<int>((gSnap.writtenBytes * 100ULL) / gSnap.totalBytes);
        }
    }

    if (gSnap.writtenBytes >= gSnap.totalBytes) {
        if (!Update.end(true)) {
            fail(String("校验失败: ") + Update.errorString());
            return;
        }
        if (gFile) gFile.close();
        gSnap.state = State::Success;
        gSnap.progressPercent = 100;
        gSnap.message = "SD刷机完成，建议重启";
        gSnap.rebootSuggested = true;
    }
}

bool SdOta::isBusy() {
    return gSnap.state == State::Running;
}

SdOta::Snapshot SdOta::snapshot() {
    return gSnap;
}
