#pragma once
#include <Arduino.h>
#include <functional>

// Command server for receiving commands from Mac desktop app.
// Listens on TCP 19821 + UDP 19822 (dual-stack for compatibility).
// Protocol: client sends JSON line + \n, server responds {"ok":true}\n.

class CmdServer {
public:
    // Command callbacks
    using AnimateCallback = std::function<void(const char* state)>;     // "happy","idle","sleep","talk"
    using TextCallback = std::function<void(const char* text, bool autoSend)>;  // text input (autoSend=true for "say")
    using NotifyCallback = std::function<void(const char* app, const char* title, const char* body)>;
    using HistoryCallback = std::function<String()>;  // returns JSON array of chat messages
    using SyncEnterCallback = std::function<String()>;               // returns snapshot JSON
    using SyncPingCallback = std::function<bool()>;                  // renew active town-sync lease
    using SyncLeaveCallback = std::function<bool(const String&)>;    // apply snapshot JSON

    void begin();
    void tick();  // Call from main loop — non-blocking

    // Register callbacks
    void onAnimate(AnimateCallback cb) { animateCb = cb; }
    void onText(TextCallback cb) { textCb = cb; }
    void onNotify(NotifyCallback cb) { notifyCb = cb; }
    void onHistory(HistoryCallback cb) { historyCb = cb; }
    void onSyncEnter(SyncEnterCallback cb) { syncEnterCb = cb; }
    void onSyncPing(SyncPingCallback cb) { syncPingCb = cb; }
    void onSyncLeave(SyncLeaveCallback cb) { syncLeaveCb = cb; }

    static constexpr uint16_t CMD_PORT = 19821;      // TCP
    static constexpr uint16_t CMD_UDP_PORT = 19822;   // UDP

private:
    static constexpr int BUF_SIZE = 4096;

    AnimateCallback animateCb;
    TextCallback textCb;
    NotifyCallback notifyCb;
    HistoryCallback historyCb;
    SyncEnterCallback syncEnterCb;
    SyncPingCallback syncPingCb;
    SyncLeaveCallback syncLeaveCb;

    bool started = false;

    void tickTCP();
    void tickUDP();
    void processCommand(const char* json, char* responseBuf, int responseBufSize);
};
