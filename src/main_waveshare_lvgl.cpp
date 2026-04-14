#if defined(CATPUTER_WAVESHARE_AMOLED_18)

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_XCA9554.h>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"
#include <ArduinoJson.h>
#include "lvgl.h"
#include "cmd_server.h"
#include "companion.h"
#include "config.h"
#include "ai_client.h"
#include "local_tts.h"
#include "pet_storage.h"
#include "serial_sd_sync.h"
#include "sprites.h"
#include "sprites_purple.h"
#include "sprites_q.h"
#include "state_broadcast.h"
#include "sd_ota.h"
#include "tts_playback.h"
#include "weather_client.h"
#include "waveshare_home_bg.h"
#include "waveshare_photo_assets.h"
#include <esp_heap_caps.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>
#include <time.h>

LV_FONT_DECLARE(lv_font_cat_cn_16);

static Adafruit_XCA9554 expander;
static Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
static Arduino_SH8601 *gfx = new Arduino_SH8601(
    bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);

static std::shared_ptr<Arduino_IIC_DriveBus> i2cBus =
    std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
static std::unique_ptr<Arduino_IIC> FT3168(new Arduino_FT3x68(
    i2cBus, FT3168_DEVICE_ADDRESS, DRIVEBUS_DEFAULT_VALUE, TP_INT,
    []() { FT3168->IIC_Interrupt_Flag = true; }));

static lv_disp_draw_buf_t drawBuf;
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;
static lv_obj_t *stageLayer = nullptr;
static lv_obj_t *petCard = nullptr;
static lv_obj_t *nameLabel = nullptr;
static lv_obj_t *stateLabel = nullptr;
static lv_obj_t *detailLabel = nullptr;
static lv_obj_t *actionLabel = nullptr;
static lv_obj_t *actionPanel = nullptr;
static lv_obj_t *actionButtonWrap = nullptr;
static lv_obj_t *quickPanel = nullptr;
static lv_obj_t *quickPanelLabel = nullptr;
static lv_obj_t *quickPanelStatsCard = nullptr;
static lv_obj_t *quickPanelStatValues[4] = {};
static lv_obj_t *settingsPanel = nullptr;
static lv_obj_t *settingsStatusLabel = nullptr;
static lv_obj_t *quickVolumeDownBtn = nullptr;
static lv_obj_t *quickVolumeUpBtn = nullptr;
static lv_obj_t *quickProvisionBtn = nullptr;
static lv_obj_t *quickProvisionBtnLabel = nullptr;
static lv_obj_t *quickRetryBtn = nullptr;
static lv_obj_t *quickOtaBtn = nullptr;
static lv_obj_t *weatherChip = nullptr;
static lv_obj_t *weatherChipLabel = nullptr;
static lv_obj_t *timeChip = nullptr;
static lv_obj_t *timeChipLabel = nullptr;
static lv_obj_t *bondChip = nullptr;
static lv_obj_t *bondChipLabel = nullptr;
static lv_obj_t *statsCard = nullptr;
static lv_obj_t *statValues[4] = {};
static lv_obj_t *actionButtons[4] = {};
static lv_obj_t *actionButtonLabels[4] = {};
static lv_obj_t *detailsPanel = nullptr;
static lv_obj_t *detailsLabel = nullptr;
static lv_obj_t *souvenirPanel = nullptr;
static lv_obj_t *souvenirTitleLabel = nullptr;
static lv_obj_t *souvenirItemLabel = nullptr;
static lv_obj_t *souvenirNoteLabel = nullptr;
static lv_obj_t *souvenirIndexLabel = nullptr;
static lv_obj_t *souvenirPhotoFrame = nullptr;
static lv_obj_t *souvenirPhotoImage = nullptr;
static lv_obj_t *souvenirPhotoLabel = nullptr;
static lv_obj_t *souvenirHintLabel = nullptr;
static lv_obj_t *ttsStatusPanel = nullptr;
static lv_obj_t *ttsStatusLabel = nullptr;
static bool showingMorePage = false;
static bool showingDetails = false;
static bool actionDrawerOpen = false;
static bool quickPanelOpen = false;
static bool settingsPanelOpen = false;
static bool wasOuting = false;
static uint8_t lastSouvenirCount = 0;
static uint8_t souvenirPanelIndex = 0;
static unsigned long lastUiUpdateMs = 0;
static unsigned long actionMessageUntilMs = 0;
static String actionDetailPages[8];
static uint8_t actionDetailPageCount = 0;
static uint8_t actionDetailPageIndex = 0;
static unsigned long actionDetailNextFlipMs = 0;
static constexpr uint8_t ACTION_DETAIL_LINES_PER_PAGE = 3;
static constexpr uint8_t ACTION_DETAIL_MAX_PAGES = 8;
static constexpr unsigned long ACTION_DETAIL_PAGE_MS = 2200;
static unsigned long souvenirPanelUntilMs = 0;
static unsigned long ttsStatusUntilMs = 0;
static bool sdReady = false;
static bool localTtsWarned = false;
static bool localTtsAutoSpeakWarned = false;
static int16_t *ttsBuffer = nullptr;
static size_t ttsMaxSamples = 0;
static M5Canvas companionCanvas(nullptr);
static Companion companion;
static LocalTTS localTts;
static TTSPlayback ttsPlayback;
static WeatherClient weatherClient;
static AIClient touchAiClient;
static SerialSDSync serialSync;
static CmdServer cmdServer;
static lv_img_dsc_t sdHomeBgDsc;
static uint8_t *sdHomeBgBuffer = nullptr;
static bool sdHomeBgLoaded = false;
static char sdHomeBgKey[16] = {0};
static bool sdHomeBgCheckedOnce = false;
static unsigned long sdHomeBgLastCheckMs = 0;
static const lv_font_t *zhFont = &lv_font_cat_cn_16;
static const lv_font_t *statNumberFont = LV_FONT_DEFAULT;
static lv_point_t lastTouchPoint = {LCD_WIDTH / 2, LCD_HEIGHT / 2};
static constexpr uint8_t BOOT_BUTTON_PIN = 0;
static constexpr uint8_t PWR_BUTTON_EXIO = 4;
static bool bootButtonPressed = false;
static bool pwrButtonPressed = false;
static bool pwrHoldAnnounced = false;
static unsigned long bootButtonDownMs = 0;
static unsigned long pwrButtonDownMs = 0;
static unsigned long lastButtonPollMs = 0;
static bool touchTracking = false;
static lv_point_t touchStartPoint = {0, 0};
static unsigned long touchStartMs = 0;
static bool syncNetworkServicesStarted = false;
static bool ntpSyncStarted = false;
static bool townSyncActive = false;
static unsigned long townSyncLeaseUntil = 0;
static constexpr unsigned long TOWN_SYNC_LEASE_MS = 15000;
static lv_point_t roomCurrentOffset = {0, 0};
static lv_point_t roomMoveStartOffset = {0, 0};
static lv_point_t roomMoveTargetOffset = {0, 0};
static unsigned long roomMoveStartMs = 0;
static unsigned long roomMoveDurationMs = 0;
static unsigned long roomActionHoldUntilMs = 0;
static String deviceId = "waveshare-touch-unknown";
static const char *DEVICE_TYPE = "waveshare_amoled_18";
static const char *DEVICE_NAME = "Catputer Touch";
static constexpr int DEVICE_CAP_TOUCH = 1;
static constexpr int DEVICE_CAP_KEYBOARD = 0;
static constexpr int DEVICE_CAP_MIC = 1;
static constexpr int DEVICE_CAP_SPEAKER = 1;
static constexpr int SD_HOME_BG_W = WAVESHARE_HOME_BG_W / 2;
static constexpr int SD_HOME_BG_H = WAVESHARE_HOME_BG_H / 2;
static constexpr uint16_t SD_HOME_BG_ZOOM = 512;
static constexpr size_t HOME_BG_BYTES = SD_HOME_BG_W * SD_HOME_BG_H * 2;
static constexpr bool SD_HOME_BACKGROUND_ENABLED = true;
static constexpr size_t SD_HOME_BACKGROUND_MIN_INTERNAL_FREE = 16 * 1024;
static constexpr size_t SD_HOME_BACKGROUND_MIN_PSRAM_BLOCK = HOME_BG_BYTES + 64 * 1024;
static constexpr unsigned long SD_HOME_BACKGROUND_CHECK_MS = 60000;
static constexpr int LVGL_DRAW_BUFFER_LINES = 32;
static constexpr bool WAVESHARE_TTS_DISABLED = true;
static constexpr unsigned long TOUCH_AI_COOLDOWN_MS = 10000;
static constexpr unsigned long TOUCH_AI_REPLY_HOLD_MS = 6200;
static constexpr unsigned long TOUCH_AI_REPLY_MAX_WAIT_MS = 28000;
static constexpr size_t TOUCH_AI_MIN_FREE_HEAP = 56 * 1024;
static constexpr size_t TOUCH_AI_MIN_LARGEST_8BIT = 72 * 1024;
static bool touchAiInFlight = false;
static bool touchAiReady = false;
static bool touchAiPendingStart = false;
static bool touchAiNeedsReconfigure = true;
static unsigned long touchAiCooldownUntilMs = 0;
static unsigned long touchAiStartedMs = 0;
static String touchAiReplyText;
static String touchAiErrorText;
static String touchAiHostApplied;
static String touchAiPortApplied;
static String touchAiTokenApplied;
static String touchAiRequestPrompt;
static unsigned long weatherUpdateLastTickMs = 0;
static constexpr unsigned long WEATHER_UPDATE_TICK_MS = 5000;
static constexpr unsigned long PROVISION_SESSION_MS = 5UL * 60UL * 1000UL;
static constexpr unsigned long PROVISION_WIFI_VERIFY_MS = 20000UL;
static constexpr uint16_t PROVISION_HTTP_PORT = 80;
static constexpr size_t PROVISION_MIN_LARGEST_8BIT = 160 * 1024;
static constexpr bool PROVISION_BLE_ENABLED = false;
static const char *PROVISION_AP_SSID = "Catputer-Setup";
static const char *PROVISION_AP_PASS = "catputer123";

enum class ProvisionState : uint8_t {
    Idle,
    BleAdvertising,
    BleConnected,
    ApplyingConfig,
    WifiVerifying,
    Success,
    Failed,
    Cancelled,
    Timeout
};

struct ProvisionContext {
    ProvisionState state = ProvisionState::Idle;
    unsigned long startedAtMs = 0;
    unsigned long expiresAtMs = 0;
    uint8_t retryCount = 0;
    String lastErrorCode;
    String lastErrorMessage;
    unsigned long wifiVerifyDeadlineMs = 0;
};

static ProvisionContext provisionCtx;
static WebServer provisionWeb(PROVISION_HTTP_PORT);
static bool provisionWebStarted = false;
static bool provisionApStarted = false;
static BLEServer *provisionBleServer = nullptr;
static BLEAdvertising *provisionBleAdvertising = nullptr;
static BLECharacteristic *provisionBleAccessChar = nullptr;
static bool provisionBleInitialized = false;
static bool provisionBleConnected = false;
static const char *PROVISION_BLE_DEVICE_NAME = "Catputer Setup";
static const char *PROVISION_BLE_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *PROVISION_BLE_CHAR_ACCESS_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

enum class PendingSwipe : uint8_t {
    None,
    Left,
    Right,
    Up,
    Down
};

static PendingSwipe pendingSwipe = PendingSwipe::None;

enum class HomeAction : uint8_t {
    Feed,
    Play,
    Clean,
    Nap,
    Out,
    Bag,
    Stats,
    Happy,
    TogglePage,
    PetTap,
    PromptChoice1,
    PromptChoice2,
    PromptChoice3,
    PromptDismiss
};

static HomeAction currentButtonActions[4] = {};
static bool roomPendingActionActive = false;
static HomeAction roomPendingAction = HomeAction::Feed;
static constexpr unsigned long ROOM_ACTION_SETTLE_MS = 18000;

struct ActionSlot {
    const char *label;
    HomeAction action;
};

static const ActionSlot pageMain[4] = {
    {"喂食", HomeAction::Feed},
    {"玩耍", HomeAction::Play},
    {"清理", HomeAction::Clean},
    {"小睡", HomeAction::Nap},
};

static const ActionSlot pageMore[4] = {
    {"外出", HomeAction::Out},
    {"纪念", HomeAction::Bag},
    {"状态", HomeAction::Stats},
    {"互动", HomeAction::Happy},
};

static void refreshUi();
static void hideSouvenirPanel();
static bool isSouvenirPanelVisible();
static void moveSouvenirPage(int delta);
static void setVolumeLevel(uint8_t value, bool persist);
static void serviceLocalTts();
static void serviceWeatherNetwork();
static void scheduleWeatherWifiRetry(unsigned long delayMs);
static void startSyncNetworkServices();
static void expireTownSyncIfNeeded();
static String buildTownSyncSnapshotJson();
static bool applyTownSyncSnapshotJson(const String& snapshotJson);
static bool isWeatherWifiTransitioning();
static void setTtsStatus(const char *text, unsigned long durationMs = 6000);
static void setActionMessage(const char *text);
static bool triggerTouchCompanionAi();
static void serviceTouchCompanionAi();
static const char *weatherName(WeatherType type);
static const char *primaryStatusText();
static const char *currentHomeBackgroundKey();
static const char *homeBackgroundPeriodLabel(const char *key);
static void discardPendingSpeech();
static void clearActionDetailPaging();
static void buildActionDetailPages(const char *text);
static void updateActionDetailPageIfNeeded();
static void serviceProvisioningSession();
static void startProvisioningSession();
static void cancelProvisioningSession();
static void retryProvisioningSession();
static bool provisionIsActive();
static const char *provisionStateLabel();
static int provisionSecondsLeft();
static void beginProvisionWebIfNeeded();
static void stopProvisionWeb();
static void beginProvisionBleIfNeeded();
static void stopProvisionBle();
static String provisionAccessAddress();
static void handleProvisionRoot();
static void handleProvisionStatus();
static void handleProvisionScan();
static void handleProvisionConfig();
static void handleProvisionStart();
static void handleProvisionCancel();
static void handleProvisionRetry();
static void triggerSdOtaFromQuickPanel();
static void onQuickPanelAction(lv_event_t *e);

static int utf8CharLen(uint8_t c) {
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static int utf8CountChars(const String &s) {
    int count = 0;
    for (int i = 0; i < (int)s.length();) {
        i += utf8CharLen((uint8_t)s[i]);
        ++count;
    }
    return count;
}

static String utf8SliceChars(const String &s, int startChar, int charCount) {
    if (charCount <= 0) return "";
    int byteStart = 0;
    int curChar = 0;
    while (byteStart < (int)s.length() && curChar < startChar) {
        byteStart += utf8CharLen((uint8_t)s[byteStart]);
        ++curChar;
    }
    int byteEnd = byteStart;
    int taken = 0;
    while (byteEnd < (int)s.length() && taken < charCount) {
        byteEnd += utf8CharLen((uint8_t)s[byteEnd]);
        ++taken;
    }
    return s.substring(byteStart, byteEnd);
}

static String cleanAiBubbleText(const String &raw) {
    String text = raw;
    text.replace("\r", "");
    text.replace("|", "\n");
    text.replace("。", "。\n");
    text.trim();
    if (text.length() == 0) return "";

    String out;
    out.reserve(240);
    int start = 0;
    while (start < static_cast<int>(text.length())) {
        int end = text.indexOf('\n', start);
        if (end < 0) end = text.length();
        String line = text.substring(start, end);
        line.trim();
        if (line.length() > 0) {
            if (out.length() > 0) out += "\n";
            out += line;
        }
        start = end + 1;
    }
    if (out.length() == 0) {
        out = text;
    }
    if (out.length() > 320) out = out.substring(0, 320);
    out.trim();
    return out;
}

static uint8_t detailCharsPerLine() {
    int detailW = min(344, max(260, LCD_WIDTH - 24));
    int chars = (detailW - 22) / 14;
    if (chars < 12) chars = 12;
    if (chars > 30) chars = 30;
    return static_cast<uint8_t>(chars);
}

static void clearActionDetailPaging() {
    actionDetailPageCount = 0;
    actionDetailPageIndex = 0;
    actionDetailNextFlipMs = 0;
    for (uint8_t i = 0; i < ACTION_DETAIL_MAX_PAGES; ++i) {
        actionDetailPages[i] = "";
    }
}

static void buildActionDetailPages(const char *text) {
    clearActionDetailPaging();
    if (!text) return;
    String input = text;
    input.replace("\r", "");
    input.trim();
    if (input.length() == 0) return;

    const int maxLines = ACTION_DETAIL_MAX_PAGES * ACTION_DETAIL_LINES_PER_PAGE;
    const int charsPerLine = detailCharsPerLine();
    String lines[maxLines];
    int lineCount = 0;

    int start = 0;
    while (start <= static_cast<int>(input.length()) && lineCount < maxLines) {
        int end = input.indexOf('\n', start);
        if (end < 0) end = input.length();
        String logical = input.substring(start, end);
        logical.trim();
        if (logical.length() == 0) {
            lines[lineCount++] = "";
        } else {
            int totalChars = utf8CountChars(logical);
            int offset = 0;
            while (offset < totalChars && lineCount < maxLines) {
                int take = min(charsPerLine, totalChars - offset);
                String seg = utf8SliceChars(logical, offset, take);
                seg.trim();
                lines[lineCount++] = seg;
                offset += take;
            }
        }
        if (end >= static_cast<int>(input.length())) break;
        start = end + 1;
    }

    if (lineCount == 0) {
        lines[0] = input;
        lineCount = 1;
    }

    int page = 0;
    for (int i = 0; i < lineCount && page < ACTION_DETAIL_MAX_PAGES; i += ACTION_DETAIL_LINES_PER_PAGE) {
        String pageText;
        for (int j = 0; j < ACTION_DETAIL_LINES_PER_PAGE && (i + j) < lineCount; ++j) {
            if (j > 0) pageText += "\n";
            pageText += lines[i + j];
        }
        pageText.trim();
        actionDetailPages[page++] = pageText;
    }

    actionDetailPageCount = static_cast<uint8_t>(page);
    if (actionDetailPageCount == 0) {
        actionDetailPageCount = 1;
        actionDetailPages[0] = input;
    }
    actionDetailPageIndex = 0;
    actionDetailNextFlipMs = (actionDetailPageCount > 1) ? (millis() + ACTION_DETAIL_PAGE_MS) : 0;
}

static void updateActionDetailPageIfNeeded() {
    if (!detailLabel || actionDetailPageCount <= 1) return;
    unsigned long now = millis();
    if (actionDetailNextFlipMs == 0 || now < actionDetailNextFlipMs) return;
    actionDetailPageIndex = static_cast<uint8_t>((actionDetailPageIndex + 1) % actionDetailPageCount);
    lv_label_set_text(detailLabel, actionDetailPages[actionDetailPageIndex].c_str());
    actionDetailNextFlipMs = now + ACTION_DETAIL_PAGE_MS;
}

static bool isUnfriendlyBubble(const String &text) {
    if (text.length() == 0) return true;
    const char *badWords[] = {
        "thirsty for data",
        "need some bytes",
        "idiot",
        "stupid",
        "闭嘴",
        "滚",
        "蠢",
        "傻",
        "无聊",
        "不想理你",
        "懒得"
    };
    String lower = text;
    lower.toLowerCase();
    for (size_t i = 0; i < sizeof(badWords) / sizeof(badWords[0]); ++i) {
        String token = badWords[i];
        token.toLowerCase();
        if (lower.indexOf(token) >= 0) return true;
    }
    return false;
}

static String fallbackTouchBubble() {
    static const char *fallbacks[] = {
        "摸摸收到，我在你身边。\n新鲜事：今天的云层很温柔，适合慢一点。",
        "被你点到就开心。\n新鲜事：晚风会把体感温度再降一点，记得加件薄外套。",
        "我在认真陪你。\n新鲜事：现在适合做一件小事，完成感会很高。",
        "你的出现是好消息。\n新鲜事：夜里空气更安静，最适合放松一下。",
        "我收到了你的摸摸。\n新鲜事：今天也是可以重新开始的一天。"
    };
    size_t idx = (millis() / 777UL) % (sizeof(fallbacks) / sizeof(fallbacks[0]));
    return String(fallbacks[idx]);
}

static void ensureTouchAiConfigured() {
    String host = Config::getGatewayHost();
    String port = Config::getGatewayPort();
    String token = Config::getGatewayToken();
    String apiKey = Config::getApiKey();

    if (host.length() == 0 && Config::getGatewayHost2().length() > 0) {
        host = Config::getGatewayHost2();
    }
    if (port.length() == 0) port = "443";
    if (token.length() == 0) token = apiKey;

    if (!touchAiNeedsReconfigure &&
        host == touchAiHostApplied &&
        port == touchAiPortApplied &&
        token == touchAiTokenApplied) {
        return;
    }

    touchAiClient.begin(apiKey, host, port, token);
    touchAiHostApplied = host;
    touchAiPortApplied = port;
    touchAiTokenApplied = token;
    touchAiNeedsReconfigure = false;
}

static bool triggerTouchCompanionAi() {
    unsigned long now = millis();
    if (touchAiInFlight) return false;
    if (static_cast<long>(now - touchAiCooldownUntilMs) < 0) return false;
    if (WiFi.status() != WL_CONNECTED) return false;
    size_t freeHeap = ESP.getFreeHeap();
    size_t largest8 = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    if (freeHeap < TOUCH_AI_MIN_FREE_HEAP || largest8 < TOUCH_AI_MIN_LARGEST_8BIT) {
        Serial.printf("[TOUCH_AI] skipped low memory: heap=%u largest8=%u\n",
                      static_cast<unsigned>(freeHeap),
                      static_cast<unsigned>(largest8));
        return false;
    }

    ensureTouchAiConfigured();
    if (touchAiHostApplied.length() == 0 || touchAiPortApplied.length() == 0 ||
        (touchAiTokenApplied.length() == 0 && Config::getApiKey().length() == 0)) {
        return false;
    }

    touchAiClient.clearHistory();
    touchAiRequestPrompt =
        String("你是温柔、有礼貌、会关心主人的口袋宠物。") +
        "我刚刚摸了摸你。请用简体中文回复2到3句短句，"
        "每句8到16字，换行分隔。"
        "语气要求：可爱、友善、鼓励，不阴阳怪气，不冒犯，不说英文俚语。"
        "内容要求：至少1句是“新鲜事”，可从天气变化、时间段氛围、生活小知识里选，"
        "要求轻松、有趣、可读性高。"
        "禁止：反问攻击、消极嘲讽、脏话、代码梗。"
        "当前状态：" + String(primaryStatusText()) +
        "，天气：" + String(weatherName(companion.getWeatherType())) +
        "，时间段：" + String(homeBackgroundPeriodLabel(currentHomeBackgroundKey())) + "。";

    touchAiInFlight = true;
    touchAiPendingStart = true;
    touchAiReady = false;
    touchAiStartedMs = now;
    touchAiCooldownUntilMs = now + TOUCH_AI_COOLDOWN_MS;
    return true;
}

static void serviceTouchCompanionAi() {
    if (touchAiPendingStart) {
        touchAiPendingStart = false;
        touchAiReplyText = "";
        touchAiErrorText = "";
        size_t beforeHeap = ESP.getFreeHeap();
        size_t beforeLargest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
        Serial.printf("[TOUCH_AI] request start: heap=%u largest8=%u\n",
                      static_cast<unsigned>(beforeHeap),
                      static_cast<unsigned>(beforeLargest));
        touchAiClient.sendMessage(
            touchAiRequestPrompt,
            [](const char *) {},
            []() {},
            [](const String &err) { touchAiErrorText = err; });
        if (touchAiErrorText.length() == 0) {
            touchAiReplyText = cleanAiBubbleText(touchAiClient.getLastResponse());
        }
        size_t afterHeap = ESP.getFreeHeap();
        size_t afterLargest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
        Serial.printf("[TOUCH_AI] request done: err=%s heap=%u largest8=%u\n",
                      touchAiErrorText.length() ? touchAiErrorText.c_str() : "none",
                      static_cast<unsigned>(afterHeap),
                      static_cast<unsigned>(afterLargest));
        touchAiInFlight = false;
        touchAiReady = true;
    }

    if (touchAiInFlight && millis() - touchAiStartedMs > TOUCH_AI_REPLY_MAX_WAIT_MS) {
        touchAiInFlight = false;
        touchAiPendingStart = false;
        touchAiReady = true;
        touchAiErrorText = "timeout";
    }

    if (!touchAiReady) return;
    touchAiReady = false;
    if (touchAiReplyText.length() > 0) {
        String reply = cleanAiBubbleText(touchAiReplyText);
        if (reply.length() == 0 || isUnfriendlyBubble(reply)) reply = fallbackTouchBubble();
        setActionMessage(reply.c_str());
        actionMessageUntilMs = millis() + TOUCH_AI_REPLY_HOLD_MS;
    } else {
        String fallback = fallbackTouchBubble();
        setActionMessage(fallback.c_str());
        actionMessageUntilMs = millis() + 3000;
    }
    touchAiReplyText = "";
    touchAiErrorText = "";
}

static void discardPendingSpeech() {
    if (!companion.hasPendingSpeech()) return;
    char drop[128];
    int guard = 0;
    while (companion.hasPendingSpeech() && guard < 8) {
        if (!companion.takePendingSpeech(drop, sizeof(drop))) break;
        ++guard;
    }
}

static const char *weatherName(WeatherType type) {
    switch (type) {
        case WeatherType::CLEAR: return "晴朗";
        case WeatherType::PARTLY_CLOUDY: return "多云";
        case WeatherType::OVERCAST: return "阴天";
        case WeatherType::FOG: return "雾";
        case WeatherType::DRIZZLE: return "毛毛雨";
        case WeatherType::RAIN: return "下雨";
        case WeatherType::THUNDER: return "雷暴";
        case WeatherType::SNOW: return "下雪";
        default: return "未知";
    }
}

static const char *stateName(CompanionState state) {
    switch (state) {
        case CompanionState::IDLE: return "发呆";
        case CompanionState::HAPPY: return "开心";
        case CompanionState::SLEEP: return "睡觉";
        case CompanionState::TALK: return "聊天";
        case CompanionState::STRETCH: return "伸懒腰";
        case CompanionState::LOOK: return "张望";
        default: return "发呆";
    }
}

static bool isPhotoSouvenir(const char *item) {
    return item && strncmp(item, "photo:", 6) == 0;
}

static const char *photoKey(const char *item) {
    return isPhotoSouvenir(item) ? item + 6 : "";
}

static const char *photoLabel(const char *key) {
    if (strcmp(key, "roof_sun") == 0) return "屋顶晒太阳";
    if (strcmp(key, "window_rain") == 0) return "窗边看雨";
    if (strcmp(key, "corner_store") == 0) return "街角小店";
    if (strcmp(key, "seaside_trip") == 0) return "海边散步";
    if (strcmp(key, "night_walk") == 0) return "夜路巡游";
    if (strcmp(key, "quiet_alley") == 0) return "安静小巷";
    return "外出照片";
}

static int uiScaleX(int value) {
    return (LCD_WIDTH * value) / 368;
}

static int uiScaleY(int value) {
    return (LCD_HEIGHT * value) / 448;
}

static int uiContentWidth() {
    return min(344, max(240, LCD_WIDTH - 24));
}

static int uiPetCardY() {
    return uiScaleY(132);
}

static bool shouldRoomWander() {
    if (companion.isTownSyncActive() || companion.isOuting()) return false;
    if (companion.hasActivePrompt() || companion.isSleeping() || companion.isFocusModeActive()) return false;
    CompanionState state = companion.getState();
    return state == CompanionState::IDLE || state == CompanionState::LOOK || state == CompanionState::STRETCH;
}

static lv_point_t roomWanderOffset() {
    if (!shouldRoomWander()) return {0, 0};
    static const lv_point_t points[] = {
        {0, 0},
        {-32, 12},
        {-18, 34},
        {26, 28},
        {38, 8},
        {10, 18},
    };
    size_t index = (millis() / 7000UL) % (sizeof(points) / sizeof(points[0]));
    return points[index];
}

static lv_point_t roomFurnitureOffset(HomeAction action) {
    switch (action) {
        case HomeAction::Feed: return {-72, 54};
        case HomeAction::Play: return {72, 54};
        case HomeAction::Clean: return {70, -32};
        case HomeAction::Nap: return {-72, -32};
        default: return {0, 0};
    }
}

static void startRoomMove(lv_point_t target, unsigned long durationMs = 850, unsigned long holdMs = 2400) {
    roomMoveStartOffset = roomCurrentOffset;
    roomMoveTargetOffset = target;
    roomMoveStartMs = millis();
    roomMoveDurationMs = max(1UL, durationMs);
    roomActionHoldUntilMs = millis() + durationMs + holdMs;
}

static void completeRoomPendingAction() {
    if (!roomPendingActionActive) return;
    roomPendingActionActive = false;
    roomActionHoldUntilMs = millis() + ROOM_ACTION_SETTLE_MS;
    switch (roomPendingAction) {
        case HomeAction::Feed:
            companion.feed(false);
            setActionMessage("已经喂饱啦");
            break;
        case HomeAction::Play:
            companion.play(false);
            setActionMessage("一起玩了一会");
            break;
        case HomeAction::Clean:
            companion.cleanUp(false);
            setActionMessage("清理完成");
            break;
        case HomeAction::Nap:
            companion.nap(false);
            setActionMessage("准备小睡");
            break;
        default:
            break;
    }
}

static lv_point_t updateRoomOffset() {
    unsigned long now = millis();
    if (roomMoveDurationMs > 0) {
        unsigned long elapsed = now - roomMoveStartMs;
        if (elapsed >= roomMoveDurationMs) {
            roomCurrentOffset = roomMoveTargetOffset;
            roomMoveDurationMs = 0;
            completeRoomPendingAction();
        } else {
            int progress = static_cast<int>((elapsed * 1000UL) / roomMoveDurationMs);
            roomCurrentOffset.x = roomMoveStartOffset.x + static_cast<lv_coord_t>(((roomMoveTargetOffset.x - roomMoveStartOffset.x) * progress) / 1000);
            roomCurrentOffset.y = roomMoveStartOffset.y + static_cast<lv_coord_t>(((roomMoveTargetOffset.y - roomMoveStartOffset.y) * progress) / 1000);
        }
    } else if (now > roomActionHoldUntilMs && shouldRoomWander()) {
        lv_point_t wander = roomWanderOffset();
        if (wander.x != roomMoveTargetOffset.x || wander.y != roomMoveTargetOffset.y) {
            startRoomMove(wander, 1400, 0);
            roomActionHoldUntilMs = 0;
        }
    }
    return roomCurrentOffset;
}

static void closeActionDrawerAfterRoomAction(HomeAction action) {
    actionDrawerOpen = false;
    quickPanelOpen = false;
    settingsPanelOpen = false;
    showingMorePage = false;
    roomPendingActionActive = true;
    roomPendingAction = action;
    startRoomMove(roomFurnitureOffset(action));
}

static const char *petKindLabel() {
    String kind = companion.getPetKind();
    if (kind == "q") return "企鹅";
    if (kind == "purple") return "紫猫";
    return "橘猫";
}

static const char *weatherCityLabel() {
    static char cityBuf[24];
    String city = Config::getCity();
    city.trim();
    if (city == "深圳" || city.equalsIgnoreCase("shenzhen")) return "深圳";
    if (city == "北京" || city.equalsIgnoreCase("beijing")) return "北京";
    if (city == "上海" || city.equalsIgnoreCase("shanghai")) return "上海";
    if (city == "广州" || city.equalsIgnoreCase("guangzhou")) return "广州";
    if (city == "杭州" || city.equalsIgnoreCase("hangzhou")) return "杭州";
    if (city == "成都" || city.equalsIgnoreCase("chengdu")) return "成都";
    if (city == "武汉" || city.equalsIgnoreCase("wuhan")) return "武汉";
    if (city.length() == 0) return "深圳";
    snprintf(cityBuf, sizeof(cityBuf), "%s", city.c_str());
    return cityBuf;
}

static int roundedTemperature() {
    float temp = companion.getTemperature();
    return temp >= 0 ? (int)(temp + 0.5f) : (int)(temp - 0.5f);
}

static lv_color_t photoColor(const char *key) {
    if (strcmp(key, "roof_sun") == 0) return lv_color_hex(0xf4b45b);
    if (strcmp(key, "window_rain") == 0) return lv_color_hex(0x6f98bf);
    if (strcmp(key, "corner_store") == 0) return lv_color_hex(0xd28a5a);
    if (strcmp(key, "seaside_trip") == 0) return lv_color_hex(0x4ea6b8);
    if (strcmp(key, "night_walk") == 0) return lv_color_hex(0x34436f);
    if (strcmp(key, "quiet_alley") == 0) return lv_color_hex(0x8c9a7c);
    return lv_color_hex(0x8796a8);
}

static const lv_img_dsc_t *photoImageForKey(const char *key) {
    if (strcmp(key, "roof_sun") == 0) return &waveshare_photo_roof_sun;
    if (strcmp(key, "window_rain") == 0) return &waveshare_photo_window_rain;
    if (strcmp(key, "corner_store") == 0) return &waveshare_photo_corner_store;
    if (strcmp(key, "seaside_trip") == 0) return &waveshare_photo_seaside_trip;
    if (strcmp(key, "night_walk") == 0) return &waveshare_photo_night_walk;
    if (strcmp(key, "quiet_alley") == 0) return &waveshare_photo_quiet_alley;
    return nullptr;
}

static const char *primaryStatusText() {
    if (companion.isTownSyncActive()) return "进城中";
    if (companion.isOuting()) return "外出中";
    if (companion.hasActivePrompt()) return "想问你";
    if (companion.isSleeping()) return "睡觉";
    if (companion.isFocusModeActive()) return "专注中";
    if (companion.getFullness() < 25) return "饿了";
    if (companion.getMood() < 25) return "不开心";
    if (companion.getCleanliness() < 25) return "脏脏";
    if (companion.getEnergy() < 20) return "困了";
    return stateName(companion.getState());
}

static const char *detailStatusText() {
    if (companion.isTownSyncActive()) return "小猫去电脑端玩了";
    if (companion.isOuting()) return "散步中，回来会带纪念品";
    if (companion.hasActivePrompt()) return companion.getActivePromptQuestion();
    if (companion.isSleeping()) return "睡一会，晚点再玩";
    if (companion.isFocusModeActive()) return "陪你专注一会";
    if (companion.getFullness() < 25) return "想吃点东西";
    if (companion.getMood() < 25) return "想被陪一小会";
    if (companion.getCleanliness() < 25) return "想洗得香香的";
    if (companion.getEnergy() < 20) return "有点犯困";
    if (companion.getState() == CompanionState::HAPPY) return "心情不错";
    return "在你身边待着";
}

static const char *faceForState(CompanionState state, int frameIndex) {
    switch (state) {
        case CompanionState::SLEEP:
            return (frameIndex % 2 == 0) ? "(-.-) zZ" : "(-.-) zz";
        case CompanionState::HAPPY:
            return (frameIndex % 2 == 0) ? "(^_^)" : "(^o^)";
        case CompanionState::TALK:
            return (frameIndex % 2 == 0) ? "(o_o)" : "(O_O)";
        case CompanionState::STRETCH:
            return "\\(^_^)/";
        case CompanionState::LOOK:
            return (frameIndex % 2 == 0) ? "(o.o)" : "(O.O)";
        default:
            return (frameIndex % 2 == 0) ? "(=^.^=)" : "(=^o^=)";
    }
}

struct LvglSpriteFrame {
    const uint16_t *pixels;
    int width;
    int height;
    int scale;
};

static LvglSpriteFrame selectSpriteFrame() {
    String kind = companion.getPetKind();
    CompanionState state = companion.getState();
    int frameIndex = companion.getFrameIndex();
    const bool purple = kind == "purple";
    const bool penguin = kind == "q";

    const int spriteW = penguin ? Q_CHAR_W : CHAR_W;
    const int spriteH = penguin ? Q_CHAR_H : CHAR_H;
    const int spriteScale = penguin ? 4 : 6;
    const uint16_t *frame = idle_frames[0];

    if (state == CompanionState::SLEEP) {
        frame = penguin ? q_sleep_frames[0] : (purple ? purple_sleep_frames[0] : sleep_frames[0]);
    } else if (state == CompanionState::HAPPY) {
        int idx = frameIndex % 2;
        frame = penguin ? q_happy_frames[idx] : (purple ? purple_happy_frames[idx] : happy_frames[idx]);
    } else if (state == CompanionState::TALK) {
        int idx = frameIndex % 2;
        frame = penguin ? q_talk_frames[idx] : (purple ? purple_talk_frames[idx] : talk_frames[idx]);
    } else {
        int idx = frameIndex % (penguin ? Q_IDLE_FRAME_COUNT : 4);
        frame = penguin ? q_idle_frames[idx] : (purple ? purple_idle_frames[idx] : idle_frames[idx]);
    }

    return {frame, spriteW, spriteH, spriteScale};
}

static lv_color_t from565(uint16_t c) {
    uint8_t r = ((c >> 11) & 0x1F) * 255 / 31;
    uint8_t g = ((c >> 5) & 0x3F) * 255 / 63;
    uint8_t b = (c & 0x1F) * 255 / 31;
    return lv_color_make(r, g, b);
}

static void drawPetSprite(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;
    lv_obj_t *obj = lv_event_get_target(e);
    lv_draw_ctx_t *drawCtx = lv_event_get_draw_ctx(e);
    if (!drawCtx) return;

    LvglSpriteFrame frame = selectSpriteFrame();
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_draw_rect_dsc_t shadowDsc;
    lv_draw_rect_dsc_init(&shadowDsc);
    shadowDsc.bg_color = lv_color_hex(0x243647);
    shadowDsc.bg_opa = LV_OPA_70;
    shadowDsc.radius = LV_RADIUS_CIRCLE;
    lv_area_t shadowArea = {
        static_cast<lv_coord_t>(coords.x1 + 64),
        static_cast<lv_coord_t>(coords.y2 - 28),
        static_cast<lv_coord_t>(coords.x2 - 64),
        static_cast<lv_coord_t>(coords.y2 - 14)};
    lv_draw_rect(drawCtx, &shadowDsc, &shadowArea);

    if (companion.isTownSyncActive()) {
        return;
    }

    const int scale = frame.scale;
    const int drawW = frame.width * scale;
    const int drawH = frame.height * scale;
    const int startX = coords.x1 + (lv_area_get_width(&coords) - drawW) / 2;
    const int startY = coords.y1 + max(8, (lv_area_get_height(&coords) - drawH) / 2 - 4);

    lv_draw_rect_dsc_t pxDsc;
    lv_draw_rect_dsc_init(&pxDsc);
    pxDsc.border_width = 0;
    pxDsc.radius = 0;
    pxDsc.bg_opa = LV_OPA_COVER;

    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.width; ++x) {
            uint16_t c = frame.pixels[y * frame.width + x];
            if (c == Color::TRANSPARENT) continue;
            pxDsc.bg_color = from565(c);
            lv_area_t px = {
                static_cast<lv_coord_t>(startX + x * scale),
                static_cast<lv_coord_t>(startY + y * scale),
                static_cast<lv_coord_t>(startX + x * scale + scale - 1),
                static_cast<lv_coord_t>(startY + y * scale + scale - 1)};
            lv_draw_rect(drawCtx, &pxDsc, &px);
        }
    }
}

static const char *defaultActionMessage() {
    if (companion.hasActivePrompt()) return companion.getActivePromptQuestion();
    if (settingsPanelOpen) return "设置界面：点大按钮操作，BOOT返回";
    if (quickPanelOpen) return "下滑信息面板：状态、音量与存储";
    if (actionDrawerOpen) return showingMorePage ? "更多动作已准备" : "主页动作已准备";
    return "上滑动作，左/右滑切换，PWR 呼出";
}

static lv_color_t weatherBgColor(WeatherType type) {
    switch (type) {
        case WeatherType::CLEAR: return lv_color_hex(0x214769);
        case WeatherType::PARTLY_CLOUDY: return lv_color_hex(0x304b67);
        case WeatherType::OVERCAST: return lv_color_hex(0x324252);
        case WeatherType::FOG: return lv_color_hex(0x485563);
        case WeatherType::DRIZZLE: return lv_color_hex(0x304a5a);
        case WeatherType::RAIN: return lv_color_hex(0x22384f);
        case WeatherType::SNOW: return lv_color_hex(0x526579);
        case WeatherType::THUNDER: return lv_color_hex(0x241f3f);
        default: return lv_color_hex(0x18314a);
    }
}

static lv_color_t weatherCardColor(WeatherType type) {
    switch (type) {
        case WeatherType::CLEAR: return lv_color_hex(0x4b78a6);
        case WeatherType::PARTLY_CLOUDY: return lv_color_hex(0x4f6f94);
        case WeatherType::OVERCAST: return lv_color_hex(0x495f77);
        case WeatherType::FOG: return lv_color_hex(0x617487);
        case WeatherType::DRIZZLE: return lv_color_hex(0x456982);
        case WeatherType::RAIN: return lv_color_hex(0x35516f);
        case WeatherType::SNOW: return lv_color_hex(0x6783a1);
        case WeatherType::THUNDER: return lv_color_hex(0x4f4c8e);
        default: return lv_color_hex(0x35516f);
    }
}

static int currentStageHour() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) return timeinfo.tm_hour;
    if (Config::getWeatherCacheValid()) return Config::getWeatherCacheIsDay() ? 12 : 22;
    return 12;
}

static const char *currentHomeBackgroundKey() {
    int hour = currentStageHour();
    if (hour >= 5 && hour < 10) return "morning";
    if (hour >= 10 && hour < 17) return "noon";
    if (hour >= 17 && hour < 20) return "dusk";
    return "night";
}

static const char *homeBackgroundPeriodLabel(const char *key) {
    if (!key) return "未知";
    if (strcmp(key, "morning") == 0) return "早晨";
    if (strcmp(key, "noon") == 0) return "白天";
    if (strcmp(key, "dusk") == 0) return "傍晚";
    if (strcmp(key, "night") == 0) return "夜晚";
    return "未知";
}

static const char *currentTimeLabel() {
    static char buf[32];
    const char *period = homeBackgroundPeriodLabel(currentHomeBackgroundKey());
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        snprintf(buf, sizeof(buf), "%02d:%02d %s", timeinfo.tm_hour, timeinfo.tm_min, period);
    } else {
        snprintf(buf, sizeof(buf), "--:-- %s", period);
    }
    return buf;
}

static String homeBackgroundPath(const char *key) {
    return String("/pet/backgrounds/home_") + (key ? key : "noon") + ".rgb565";
}

static String homeBackgroundReadyPath(const char *key) {
    return String("/pet/backgrounds/home_") + (key ? key : "noon") + ".ok";
}

static bool loadHomeBackgroundFromSd(const char *key) {
    if (!SD_HOME_BACKGROUND_ENABLED) return false;
    if (!PetStorage::isAvailable()) return false;

    size_t internalFree = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t largestPsram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (internalFree < SD_HOME_BACKGROUND_MIN_INTERNAL_FREE || largestPsram < SD_HOME_BACKGROUND_MIN_PSRAM_BLOCK) {
        Serial.printf("[BG] SD background skipped: internal=%u largestPsram=%u\n",
                      static_cast<unsigned>(internalFree),
                      static_cast<unsigned>(largestPsram));
        return false;
    }

    String path = homeBackgroundPath(key);
    String readyPath = homeBackgroundReadyPath(key);
    if (!PetStorage::fs().exists(readyPath)) {
        Serial.printf("[BG] SD background not ready: %s\n", path.c_str());
        return false;
    }
    if (!PetStorage::fs().exists(path)) {
        Serial.printf("[BG] SD background missing: %s\n", path.c_str());
        return false;
    }

    File file = PetStorage::fs().open(path, FILE_READ);
    if (!file) {
        Serial.printf("[BG] SD background open failed: %s\n", path.c_str());
        return false;
    }
    if (file.size() != HOME_BG_BYTES) {
        Serial.printf("[BG] SD background size mismatch: %s size=%u expected=%u\n",
                      path.c_str(), static_cast<unsigned>(file.size()), static_cast<unsigned>(HOME_BG_BYTES));
        file.close();
        return false;
    }

    if (!sdHomeBgBuffer) {
        sdHomeBgBuffer = static_cast<uint8_t *>(heap_caps_malloc(HOME_BG_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }
    if (!sdHomeBgBuffer) {
        Serial.println("[BG] SD background PSRAM allocation failed");
        file.close();
        return false;
    }

    size_t readBytes = file.read(sdHomeBgBuffer, HOME_BG_BYTES);
    file.close();
    if (readBytes != HOME_BG_BYTES) {
        Serial.printf("[BG] SD background read failed: %s read=%u\n",
                      path.c_str(), static_cast<unsigned>(readBytes));
        return false;
    }

    sdHomeBgDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    sdHomeBgDsc.header.always_zero = 0;
    sdHomeBgDsc.header.reserved = 0;
    sdHomeBgDsc.header.w = SD_HOME_BG_W;
    sdHomeBgDsc.header.h = SD_HOME_BG_H;
    sdHomeBgDsc.data_size = HOME_BG_BYTES;
    sdHomeBgDsc.data = sdHomeBgBuffer;
    strncpy(sdHomeBgKey, key ? key : "", sizeof(sdHomeBgKey) - 1);
    sdHomeBgKey[sizeof(sdHomeBgKey) - 1] = '\0';
    sdHomeBgLoaded = true;
    if (stageLayer) lv_obj_invalidate(stageLayer);
    Serial.printf("[BG] Loaded SD background: key=%s hour=%d path=%s\n",
                  key ? key : "", currentStageHour(), path.c_str());
    return true;
}

static void updateHomeBackgroundFromSd(bool force = false) {
    unsigned long now = millis();
    if (!force && sdHomeBgCheckedOnce && now - sdHomeBgLastCheckMs < SD_HOME_BACKGROUND_CHECK_MS) return;
    if (sdHomeBgLoaded &&
        (isWeatherWifiTransitioning() ||
        ttsPlayback.isPlaying() ||
        ttsBuffer != nullptr)) {
        return;
    }
    sdHomeBgCheckedOnce = true;
    sdHomeBgLastCheckMs = now;

    const char *key = currentHomeBackgroundKey();
    if (sdHomeBgLoaded && strcmp(sdHomeBgKey, key) == 0) return;
    Serial.printf("[BG] Selecting background: key=%s hour=%d\n", key, currentStageHour());
    if (!loadHomeBackgroundFromSd(key) && !sdHomeBgLoaded) {
        sdHomeBgKey[0] = '\0';
    }
}

static const lv_img_dsc_t *currentHomeBackground() {
    if (sdHomeBgLoaded) return &sdHomeBgDsc;
    return &waveshare_home_bg;
}

static void drawStageBackground(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;
    lv_obj_t *obj = lv_event_get_target(e);
    lv_draw_ctx_t *drawCtx = lv_event_get_draw_ctx(e);
    if (!drawCtx) return;

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    lv_draw_img_dsc_t bgDsc;
    lv_draw_img_dsc_init(&bgDsc);
    if (sdHomeBgLoaded) {
        bgDsc.zoom = SD_HOME_BG_ZOOM;
        bgDsc.antialias = 0;
        lv_area_t srcCoords = {
            coords.x1,
            coords.y1,
            static_cast<lv_coord_t>(coords.x1 + SD_HOME_BG_W - 1),
            static_cast<lv_coord_t>(coords.y1 + SD_HOME_BG_H - 1)};
        lv_draw_img(drawCtx, &bgDsc, &srcCoords, currentHomeBackground());
        return;
    }
    lv_draw_img(drawCtx, &bgDsc, &coords, currentHomeBackground());
    return;

    int w = lv_area_get_width(&coords);
    int h = lv_area_get_height(&coords);
    int skyHeight = max(180, (h * 62) / 100);
    int groundY = coords.y1 + min(h - 80, skyHeight);
    int hour = (millis() / 60000UL) % 24;

    lv_color_t sky = lv_color_hex(0x6da7de);
    lv_color_t ground = lv_color_hex(0x50663a);
    lv_color_t groundTop = lv_color_hex(0x90bd5b);
    if (hour >= 17 && hour < 19) {
        sky = lv_color_hex(0xd57d69);
        ground = lv_color_hex(0x5d5636);
        groundTop = lv_color_hex(0xcaa56b);
    } else if (hour >= 19 || hour < 6) {
        sky = lv_color_hex(0x18233c);
        ground = lv_color_hex(0x22362d);
        groundTop = lv_color_hex(0x476153);
    }

    lv_draw_rect_dsc_t rectDsc;
    lv_draw_rect_dsc_init(&rectDsc);
    rectDsc.border_width = 0;
    rectDsc.radius = 0;
    rectDsc.bg_opa = LV_OPA_COVER;

    lv_area_t skyArea = {coords.x1, coords.y1, coords.x2, static_cast<lv_coord_t>(groundY)};
    rectDsc.bg_color = sky;
    lv_draw_rect(drawCtx, &rectDsc, &skyArea);

    if (hour >= 19 || hour < 6) {
        rectDsc.bg_color = lv_color_hex(0xf0efc6);
        rectDsc.radius = LV_RADIUS_CIRCLE;
        lv_area_t moon = {coords.x1 + 24, coords.y1 + 18, coords.x1 + 44, coords.y1 + 38};
        lv_draw_rect(drawCtx, &rectDsc, &moon);
        rectDsc.bg_color = sky;
        lv_area_t moonCut = {coords.x1 + 30, coords.y1 + 14, coords.x1 + 48, coords.y1 + 36};
        lv_draw_rect(drawCtx, &rectDsc, &moonCut);
        rectDsc.radius = 0;
        rectDsc.bg_color = lv_color_hex(0xfff7de);
        for (int i = 0; i < 8; ++i) {
            int sx = coords.x1 + 18 + ((i * 39 + (millis() / 150) % 19) % (w - 36));
            int sy = coords.y1 + 14 + ((i * 17) % 80);
            lv_area_t star = {static_cast<lv_coord_t>(sx), static_cast<lv_coord_t>(sy), static_cast<lv_coord_t>(sx + 1), static_cast<lv_coord_t>(sy + 1)};
            lv_draw_rect(drawCtx, &rectDsc, &star);
        }
    } else {
        rectDsc.bg_color = (hour >= 17) ? lv_color_hex(0xffb45b) : lv_color_hex(0xffe07e);
        rectDsc.radius = LV_RADIUS_CIRCLE;
        lv_area_t sun = {coords.x2 - 52, coords.y1 + 16, coords.x2 - 26, coords.y1 + 42};
        lv_draw_rect(drawCtx, &rectDsc, &sun);
        rectDsc.radius = 0;
        rectDsc.bg_color = lv_color_hex(0xe6f1ff);
        lv_area_t cloud1 = {coords.x1 + 34, coords.y1 + 26, coords.x1 + 82, coords.y1 + 42};
        lv_draw_rect(drawCtx, &rectDsc, &cloud1);
        lv_area_t cloud2 = {coords.x1 + 210, coords.y1 + 44, coords.x1 + 250, coords.y1 + 56};
        lv_draw_rect(drawCtx, &rectDsc, &cloud2);
    }

    switch (companion.getWeatherType()) {
        case WeatherType::OVERCAST:
            rectDsc.bg_color = lv_color_hex(0x8893a4);
            rectDsc.bg_opa = LV_OPA_30;
            lv_draw_rect(drawCtx, &rectDsc, &skyArea);
            break;
        case WeatherType::DRIZZLE:
        case WeatherType::RAIN:
        case WeatherType::THUNDER: {
            lv_draw_line_dsc_t lineDsc;
            lv_draw_line_dsc_init(&lineDsc);
            lineDsc.color = lv_color_hex(0x9db8d8);
            lineDsc.width = 1;
            int offset = (millis() / 18) % 18;
            int count = (companion.getWeatherType() == WeatherType::DRIZZLE) ? 10 : 18;
            for (int i = 0; i < count; ++i) {
                int x = coords.x1 + ((i * 21 + offset * 3) % (w - 10));
                int skyEffectHeight = max(80, groundY - coords.y1 - 20);
                int y = coords.y1 + ((i * 13 + offset * 2) % skyEffectHeight);
                lv_point_t pts[2] = {
                    {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)},
                    {static_cast<lv_coord_t>(x - 3), static_cast<lv_coord_t>(y + ((companion.getWeatherType() == WeatherType::DRIZZLE) ? 8 : 13))}};
                lv_draw_line(drawCtx, &lineDsc, &pts[0], &pts[1]);
            }
            rectDsc.bg_opa = LV_OPA_20;
            rectDsc.bg_color = lv_color_hex(0x495566);
            lv_draw_rect(drawCtx, &rectDsc, &skyArea);
            rectDsc.bg_opa = LV_OPA_COVER;
            break;
        }
        case WeatherType::SNOW:
            rectDsc.bg_color = lv_color_hex(0xf7fbff);
            for (int i = 0; i < 16; ++i) {
                int sx = coords.x1 + ((i * 23 + (millis() / 40) + (i % 3) * 9) % (w - 6));
                int snowHeight = max(80, groundY - coords.y1 - 12);
                int sy = coords.y1 + ((i * 15 + (millis() / 50)) % snowHeight);
                lv_area_t flake = {static_cast<lv_coord_t>(sx), static_cast<lv_coord_t>(sy), static_cast<lv_coord_t>(sx + 1), static_cast<lv_coord_t>(sy + 1)};
                lv_draw_rect(drawCtx, &rectDsc, &flake);
            }
            break;
        case WeatherType::FOG:
            rectDsc.bg_color = lv_color_hex(0xb6bec6);
            rectDsc.bg_opa = LV_OPA_20;
            for (int i = 0; i < 4; ++i) {
                lv_area_t fog = {coords.x1 + 12, static_cast<lv_coord_t>(coords.y1 + 48 + i * 34), coords.x2 - 12, static_cast<lv_coord_t>(coords.y1 + 66 + i * 34)};
                lv_draw_rect(drawCtx, &rectDsc, &fog);
            }
            rectDsc.bg_opa = LV_OPA_COVER;
            break;
        default:
            break;
    }

    rectDsc.radius = 0;
    rectDsc.bg_color = ground;
    lv_area_t groundArea = {coords.x1, static_cast<lv_coord_t>(groundY), coords.x2, coords.y2};
    lv_draw_rect(drawCtx, &rectDsc, &groundArea);

    rectDsc.bg_color = groundTop;
    lv_area_t groundTopArea = {coords.x1, static_cast<lv_coord_t>(groundY), coords.x2, static_cast<lv_coord_t>(groundY + 2)};
    lv_draw_rect(drawCtx, &rectDsc, &groundTopArea);
    for (int i = 0; i < 10; ++i) {
        int gx = coords.x1 + ((i * 31 + 10) % (w - 8));
        lv_area_t grass = {static_cast<lv_coord_t>(gx), static_cast<lv_coord_t>(groundY + 6 + (i % 2) * 3), static_cast<lv_coord_t>(gx + 1), static_cast<lv_coord_t>(groundY + 7 + (i % 2) * 3)};
        lv_draw_rect(drawCtx, &rectDsc, &grass);
    }
}

static void setActionMessage(const char *text) {
    buildActionDetailPages(text);
    if (detailLabel) {
        if (actionDetailPageCount > 0) {
            lv_label_set_text(detailLabel, actionDetailPages[0].c_str());
            lv_obj_clear_flag(detailLabel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(detailLabel, "");
            lv_obj_add_flag(detailLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }
    unsigned long holdMs = 2400;
    if (actionDetailPageCount > 1) {
        holdMs = static_cast<unsigned long>(actionDetailPageCount) * ACTION_DETAIL_PAGE_MS + 1400;
    }
    actionMessageUntilMs = millis() + holdMs;
}

static void setTtsStatus(const char *text, unsigned long durationMs) {
    if (ttsStatusLabel) {
        lv_label_set_text(ttsStatusLabel, text ? text : "");
    }
    if (ttsStatusPanel) {
        lv_obj_clear_flag(ttsStatusPanel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ttsStatusPanel);
    }
    ttsStatusUntilMs = millis() + durationMs;
}

static void setSettingsStatus(const char *text) {
    if (settingsStatusLabel) {
        lv_label_set_text(settingsStatusLabel, text ? text : "");
    }
}

static bool provisionIsActive() {
    switch (provisionCtx.state) {
        case ProvisionState::BleAdvertising:
        case ProvisionState::BleConnected:
        case ProvisionState::ApplyingConfig:
        case ProvisionState::WifiVerifying:
            return true;
        default:
            return false;
    }
}

static String provisionAccessAddress() {
    if (provisionApStarted) {
        IPAddress apIp = WiFi.softAPIP();
        if (apIp[0] != 0) return String("http://") + apIp.toString();
    }
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress ip = WiFi.localIP();
        if (ip[0] != 0) return String("http://") + ip.toString();
    }
    return String("http://catputer.local");
}

class ProvisionBleServerCallbacks : public BLEServerCallbacks {
public:
    void onConnect(BLEServer *server) override {
        (void)server;
        provisionBleConnected = true;
        provisionCtx.state = ProvisionState::BleConnected;
    }

    void onDisconnect(BLEServer *server) override {
        (void)server;
        provisionBleConnected = false;
        if (provisionIsActive()) provisionCtx.state = ProvisionState::BleAdvertising;
        if (provisionBleAdvertising) provisionBleAdvertising->start();
    }
};

static ProvisionBleServerCallbacks provisionBleCallbacks;

static const char *provisionStateLabel() {
    switch (provisionCtx.state) {
        case ProvisionState::Idle:
            return "未开启";
        case ProvisionState::BleAdvertising:
            return "等待蓝牙连接";
        case ProvisionState::BleConnected:
            return "蓝牙已连接";
        case ProvisionState::ApplyingConfig:
            return "正在写入配置";
        case ProvisionState::WifiVerifying:
            return "正在验证网络";
        case ProvisionState::Success:
            return "配置成功";
        case ProvisionState::Failed:
            return "配置失败";
        case ProvisionState::Cancelled:
            return "已取消";
        case ProvisionState::Timeout:
            return "配网超时";
        default:
            return "未知";
    }
}

static int provisionSecondsLeft() {
    if (!provisionIsActive()) return 0;
    unsigned long now = millis();
    if (now >= provisionCtx.expiresAtMs) return 0;
    return static_cast<int>((provisionCtx.expiresAtMs - now + 999UL) / 1000UL);
}

static void handleProvisionRoot() {
    provisionCtx.state = ProvisionState::BleConnected;
    String html;
    html.reserve(4200);
    html += "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Catputer 配网</title><style>body{font-family:system-ui;margin:16px;background:#0f1724;color:#e8f1ff}input,select{box-sizing:border-box;width:100%;padding:12px;margin:7px 0 12px;border-radius:8px;border:1px solid #52708f;background:#0d1b2a;color:#fff;font-size:16px}button{padding:12px 16px;border:0;border-radius:8px;background:#4f84c4;color:#fff;font-size:16px}small{color:#9db3cb}.row{display:flex;gap:8px}.row button{flex:1}.muted{color:#9db3cb}</style></head><body>";
    html += "<h3>Catputer 配网</h3><p><small>先扫附近 Wi-Fi，选中后只需要输入密码。</small></p>";
    html += "<form method='POST' action='/api/provision/config'>";
    html += "<label>附近 Wi-Fi</label><select id='ssidList'><option value=''>正在扫描...</option></select>";
    html += "<div class='row'><button type='button' onclick='scanWifi()'>重新扫描</button><button type='button' onclick='useManual()'>手动输入</button></div>";
    html += "<label>Wi-Fi SSID</label><input id='ssid' name='ssid' required placeholder='选择或输入Wi-Fi名称'>";
    html += "<label>Wi-Fi 密码</label><input name='password' type='password'>";
    html += "<label>城市（可选）</label><input name='city' placeholder='Shenzhen'>";
    html += "<button type='submit'>保存并验证</button></form>";
    html += "<p id='scanState' class='muted'>准备扫描...</p><p><small>状态接口：/api/provision/status</small></p>";
    html += "<script>";
    html += "const list=document.getElementById('ssidList'),ssid=document.getElementById('ssid'),state=document.getElementById('scanState');";
    html += "list.onchange=()=>{ssid.value=list.value};";
    html += "function useManual(){ssid.focus();ssid.select();}";
    html += "async function scanWifi(){state.textContent='扫描中...';list.innerHTML='<option value=\"\">扫描中...</option>';try{const r=await fetch('/api/provision/scan');const j=await r.json();list.innerHTML='';if(!j.networks||j.networks.length===0){list.innerHTML='<option value=\"\">未找到，请手动输入</option>';state.textContent='未找到Wi-Fi';return;}for(const n of j.networks){const o=document.createElement('option');o.value=n.ssid;o.textContent=n.ssid+'  '+n.rssi+'dBm';list.appendChild(o);}ssid.value=list.value;state.textContent='扫描到 '+j.networks.length+' 个Wi-Fi';}catch(e){list.innerHTML='<option value=\"\">扫描失败，请手动输入</option>';state.textContent='扫描失败：'+e;}}";
    html += "scanWifi();";
    html += "</script></body></html>";
    provisionWeb.send(200, "text/html; charset=utf-8", html);
}

static void handleProvisionStatus() {
    JsonDocument doc;
    doc["ok"] = true;
    doc["state"] = provisionStateLabel();
    doc["active"] = provisionIsActive();
    doc["seconds_left"] = provisionSecondsLeft();
    doc["error_code"] = provisionCtx.lastErrorCode;
    doc["error_message"] = provisionCtx.lastErrorMessage;
    doc["access"] = provisionAccessAddress();
    String out;
    serializeJson(doc, out);
    provisionWeb.send(200, "application/json; charset=utf-8", out);
}

static void handleProvisionScan() {
    WiFi.mode(WIFI_AP_STA);
    int found = WiFi.scanNetworks(false, true);
    JsonDocument doc;
    doc["ok"] = found >= 0;
    JsonArray networks = doc["networks"].to<JsonArray>();
    if (found > 0) {
        struct ScanEntry {
            String ssid;
            int32_t rssi;
        };
        ScanEntry entries[12];
        int kept = 0;
        for (int i = 0; i < found; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0) continue;
            bool duplicate = false;
            for (int j = 0; j < kept; j++) {
                if (entries[j].ssid == ssid) {
                    duplicate = true;
                    if (WiFi.RSSI(i) > entries[j].rssi) entries[j].rssi = WiFi.RSSI(i);
                    break;
                }
            }
            if (duplicate) continue;
            int insertAt = kept;
            while (insertAt > 0 && entries[insertAt - 1].rssi < WiFi.RSSI(i)) {
                if (insertAt < 12) entries[insertAt] = entries[insertAt - 1];
                insertAt--;
            }
            if (insertAt >= 12) continue;
            entries[insertAt].ssid = ssid;
            entries[insertAt].rssi = WiFi.RSSI(i);
            if (kept < 12) kept++;
        }
        for (int i = 0; i < kept; i++) {
            JsonObject item = networks.add<JsonObject>();
            item["ssid"] = entries[i].ssid;
            item["rssi"] = entries[i].rssi;
        }
    }
    WiFi.scanDelete();
    String out;
    serializeJson(doc, out);
    provisionWeb.send(200, "application/json; charset=utf-8", out);
}

static void handleProvisionConfig() {
    String ssid = provisionWeb.arg("ssid");
    String password = provisionWeb.arg("password");
    String city = provisionWeb.arg("city");
    ssid.trim();
    city.trim();
    if (ssid.length() == 0) {
        provisionCtx.state = ProvisionState::Failed;
        provisionCtx.lastErrorCode = "BAD_SSID";
        provisionCtx.lastErrorMessage = "SSID 不能为空";
        provisionWeb.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"ssid required\"}");
        return;
    }

    provisionCtx.state = ProvisionState::ApplyingConfig;
    Config::setSSID(ssid);
    Config::setPassword(password);
    if (city.length() > 0) Config::setCity(city);
    Config::save();

    provisionCtx.state = ProvisionState::WifiVerifying;
    provisionCtx.wifiVerifyDeadlineMs = millis() + PROVISION_WIFI_VERIFY_MS;
    provisionCtx.lastErrorCode = "";
    provisionCtx.lastErrorMessage = "";

    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(false, false);
    delay(20);
    WiFi.begin(ssid.c_str(), password.c_str());

    setActionMessage("已保存，正在验证网络");
    provisionWeb.send(200, "application/json; charset=utf-8", "{\"ok\":true,\"message\":\"verifying\"}");
}

static void handleProvisionStart() {
    if (!provisionIsActive()) startProvisioningSession();
    JsonDocument doc;
    doc["ok"] = true;
    doc["state"] = provisionStateLabel();
    doc["access"] = provisionAccessAddress();
    String out;
    serializeJson(doc, out);
    provisionWeb.send(200, "application/json; charset=utf-8", out);
}

static void handleProvisionCancel() {
    cancelProvisioningSession();
    provisionWeb.send(200, "application/json; charset=utf-8", "{\"ok\":true,\"state\":\"cancelled\"}");
}

static void handleProvisionRetry() {
    retryProvisioningSession();
    JsonDocument doc;
    doc["ok"] = true;
    doc["state"] = provisionStateLabel();
    doc["active"] = provisionIsActive();
    String out;
    serializeJson(doc, out);
    provisionWeb.send(200, "application/json; charset=utf-8", out);
}

static void triggerSdOtaFromQuickPanel() {
    auto snap = SdOta::snapshot();
    if (snap.state == SdOta::State::Running) {
        setActionMessage("SD刷机进行中");
        setSettingsStatus("SD刷机进行中");
        return;
    }
    if (snap.rebootSuggested) {
        setActionMessage("正在重启应用新固件");
        setSettingsStatus("正在重启应用新固件");
        delay(120);
        ESP.restart();
        return;
    }
    bool ok = SdOta::start("/firmware/update.bin");
    auto now = SdOta::snapshot();
    if (ok) {
        setActionMessage("开始SD刷机");
        setSettingsStatus("开始SD刷机");
    } else {
        setActionMessage(now.message.c_str());
        setSettingsStatus(now.message.c_str());
    }
}

static void beginProvisionWebIfNeeded() {
    if (!provisionApStarted) {
        WiFi.mode(WIFI_AP_STA);
        provisionApStarted = WiFi.softAP(PROVISION_AP_SSID, PROVISION_AP_PASS);
    }
    if (!provisionWebStarted) {
        provisionWeb.on("/", HTTP_GET, handleProvisionRoot);
        provisionWeb.on("/api/provision/status", HTTP_GET, handleProvisionStatus);
        provisionWeb.on("/api/provision/scan", HTTP_GET, handleProvisionScan);
        provisionWeb.on("/api/provision/start", HTTP_POST, handleProvisionStart);
        provisionWeb.on("/api/provision/config", HTTP_POST, handleProvisionConfig);
        provisionWeb.on("/api/provision/cancel", HTTP_POST, handleProvisionCancel);
        provisionWeb.on("/api/provision/retry", HTTP_POST, handleProvisionRetry);
        provisionWeb.begin();
        provisionWebStarted = true;
    }
}

static void beginProvisionBleIfNeeded() {
    if (!PROVISION_BLE_ENABLED) {
        Serial.println("[PROVISION] BLE disabled on Waveshare build; web setup only.");
        return;
    }
    if (!provisionBleInitialized) {
        BLEDevice::init(PROVISION_BLE_DEVICE_NAME);
        provisionBleServer = BLEDevice::createServer();
        provisionBleServer->setCallbacks(&provisionBleCallbacks);
        BLEService *service = provisionBleServer->createService(BLEUUID(PROVISION_BLE_SERVICE_UUID));
        provisionBleAccessChar = service->createCharacteristic(
            BLEUUID(PROVISION_BLE_CHAR_ACCESS_UUID),
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
        provisionBleAccessChar->setValue(provisionAccessAddress().c_str());
        service->start();
        provisionBleAdvertising = provisionBleServer->getAdvertising();
        provisionBleAdvertising->addServiceUUID(service->getUUID());
        provisionBleInitialized = true;
    }
    if (provisionBleAccessChar) {
        provisionBleAccessChar->setValue(provisionAccessAddress().c_str());
    }
    if (provisionBleAdvertising) {
        provisionBleAdvertising->start();
    }
    provisionBleConnected = false;
}

static void stopProvisionWeb() {
    if (provisionWebStarted) {
        provisionWeb.stop();
        provisionWebStarted = false;
    }
    if (provisionApStarted) {
        WiFi.softAPdisconnect(true);
        provisionApStarted = false;
    }
}

static void stopProvisionBle() {
    if (!provisionBleInitialized) return;
    provisionBleConnected = false;
    if (provisionBleAdvertising) provisionBleAdvertising->stop();
}

static void startProvisioningSession() {
    unsigned long now = millis();
    size_t largest8 = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    size_t freeHeap = ESP.getFreeHeap();
    Serial.printf("[PROVISION] start requested heap=%u largest8=%u\n",
                  static_cast<unsigned>(freeHeap),
                  static_cast<unsigned>(largest8));
    if (largest8 < PROVISION_MIN_LARGEST_8BIT) {
        provisionCtx.state = ProvisionState::Failed;
        provisionCtx.lastErrorCode = "LOW_MEMORY";
        provisionCtx.lastErrorMessage = "内存不足，请稍后再试";
        setActionMessage("内存不足，暂不能开启配网");
        setSettingsStatus("配网未开启：内存不足");
        return;
    }
    provisionCtx.state = ProvisionState::BleAdvertising;
    provisionCtx.startedAtMs = now;
    provisionCtx.expiresAtMs = now + PROVISION_SESSION_MS;
    provisionCtx.wifiVerifyDeadlineMs = 0;
    provisionCtx.lastErrorCode = "";
    provisionCtx.lastErrorMessage = "";
    setSettingsStatus("正在开启配网...");
    beginProvisionWebIfNeeded();
    if (PROVISION_BLE_ENABLED) beginProvisionBleIfNeeded();
    String msg = PROVISION_BLE_ENABLED
                     ? String("蓝牙配网已开启，设备名: ") + PROVISION_BLE_DEVICE_NAME
                     : String("配网入口已开启，WiFi: ") + PROVISION_AP_SSID + " 访问 " + provisionAccessAddress();
    setActionMessage(msg.c_str());
    setSettingsStatus(msg.c_str());
}

static void cancelProvisioningSession() {
    provisionCtx.state = ProvisionState::Cancelled;
    provisionCtx.expiresAtMs = 0;
    provisionCtx.wifiVerifyDeadlineMs = 0;
    stopProvisionWeb();
    stopProvisionBle();
    setActionMessage("已取消蓝牙配网");
    setSettingsStatus("已取消蓝牙配网");
}

static void retryProvisioningSession() {
    setSettingsStatus("正在重试...");
    if (provisionCtx.state == ProvisionState::Failed ||
        provisionCtx.state == ProvisionState::Timeout ||
        provisionCtx.state == ProvisionState::Cancelled) {
        provisionCtx.retryCount = static_cast<uint8_t>(min<int>(255, provisionCtx.retryCount + 1));
        startProvisioningSession();
        setActionMessage("已重试蓝牙配网");
        setSettingsStatus("已重试蓝牙配网");
    } else {
        setActionMessage("当前无需重试");
        setSettingsStatus("当前无需重试");
    }
}

static void serviceProvisioningSession() {
    if (provisionWebStarted) provisionWeb.handleClient();

    if (provisionCtx.state == ProvisionState::WifiVerifying) {
        wl_status_t st = WiFi.status();
        if (st == WL_CONNECTED) {
            provisionCtx.state = ProvisionState::Success;
            provisionCtx.wifiVerifyDeadlineMs = 0;
            provisionCtx.lastErrorCode = "";
            provisionCtx.lastErrorMessage = "";
            stopProvisionWeb();
            stopProvisionBle();
            setActionMessage("配置成功，网络已连接");
            return;
        }
        if (millis() > provisionCtx.wifiVerifyDeadlineMs) {
            provisionCtx.state = ProvisionState::Failed;
            provisionCtx.lastErrorCode = "WIFI_VERIFY_TIMEOUT";
            provisionCtx.lastErrorMessage = "连接超时，请检查密码";
            provisionCtx.wifiVerifyDeadlineMs = 0;
            setActionMessage("网络验证超时");
            return;
        }
    }
    if (provisionBleAccessChar) {
        provisionBleAccessChar->setValue(provisionAccessAddress().c_str());
    }

    if (!provisionIsActive()) return;
    unsigned long now = millis();
    if (now >= provisionCtx.expiresAtMs) {
        provisionCtx.state = ProvisionState::Timeout;
        provisionCtx.lastErrorCode = "SESSION_TIMEOUT";
        provisionCtx.lastErrorMessage = "配网超时，请重试";
        provisionCtx.wifiVerifyDeadlineMs = 0;
        stopProvisionWeb();
        stopProvisionBle();
        setActionMessage("配网超时，请重试");
    }
}

static uint8_t safeVolume(uint8_t value) {
    if (value > 100) return 35;
    return value > 60 ? 60 : value;
}

static void setVolumeLevel(uint8_t value, bool persist) {
    uint8_t level = safeVolume(value);
    Config::setSpeakerVolume(level);
    M5.Speaker.setVolume(level);
    if (persist) Config::save();
}

static void adjustVolume(int delta) {
    int current = static_cast<int>(safeVolume(Config::getSpeakerVolume()));
    int next = current + delta;
    if (next < 0) next = 0;
    if (next > 60) next = 60;
    setVolumeLevel(static_cast<uint8_t>(next), true);
    companion.playKeyClick();
    char msg[48];
    snprintf(msg, sizeof(msg), "VOL %d/60", next);
    setActionMessage(msg);
    refreshUi();
}

enum class WeatherWifiState : uint8_t {
    Idle,
    ConnectingPrimary,
    ConnectingSecondary,
    Online,
    RetryLater
};

static WeatherWifiState weatherWifiState = WeatherWifiState::Idle;
static unsigned long weatherWifiStartedMs = 0;
static unsigned long weatherWifiRetryAtMs = 0;
static bool weatherCachedThisSession = false;
static String weatherClientCity;
static bool weatherClientReady = false;
static constexpr unsigned long WEATHER_CONNECT_TIMEOUT_MS = 20000;
static constexpr unsigned long WEATHER_RETRY_DELAY_MS = 15000;
static constexpr long WAVESHARE_GMT_OFFSET_SEC = 8 * 60 * 60;
static constexpr int WAVESHARE_DAYLIGHT_OFFSET_SEC = 0;
static const char *WAVESHARE_NTP_SERVER = "pool.ntp.org";

static bool hasWeatherWifiConfig() {
    return Config::getSSID().length() > 0 || Config::getSSID2().length() > 0;
}

static bool isWeatherWifiTransitioning() {
    return weatherWifiState == WeatherWifiState::ConnectingPrimary ||
           weatherWifiState == WeatherWifiState::ConnectingSecondary;
}

static void ensureWeatherClientCity() {
    String city = Config::getCity();
    if (city.length() == 0) city = "Shenzhen";
    if (weatherClientReady && city == weatherClientCity) return;

    weatherClientCity = city;
    weatherClient.begin(weatherClientCity);
    weatherClientReady = true;
}

static bool startWeatherWifi(bool secondary) {
    const String& ssid = secondary ? Config::getSSID2() : Config::getSSID();
    const String& pass = secondary ? Config::getPassword2() : Config::getPassword();
    if (ssid.length() == 0) return false;

    WiFi.persistent(false);
    bool wifiModeOk = WiFi.mode(WIFI_STA);
    Serial.printf("[WEATHER] WiFi mode: ok=%d mode=%d internal=%u\n",
                  wifiModeOk ? 1 : 0,
                  static_cast<int>(WiFi.getMode()),
                  heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid.c_str(), pass.c_str());
    weatherWifiStartedMs = millis();
    weatherWifiState = secondary ? WeatherWifiState::ConnectingSecondary : WeatherWifiState::ConnectingPrimary;
    Serial.printf("[WEATHER] WiFi connecting: %s\n", ssid.c_str());
    return true;
}

static void scheduleWeatherWifiRetry(unsigned long delayMs) {
    weatherWifiState = WeatherWifiState::RetryLater;
    weatherWifiRetryAtMs = millis() + delayMs;
    Serial.printf("[WEATHER] WiFi retry scheduled in %lu ms\n", delayMs);
}

static void ensureNtpSyncStarted() {
    if (ntpSyncStarted || WiFi.status() != WL_CONNECTED) return;
    configTime(WAVESHARE_GMT_OFFSET_SEC, WAVESHARE_DAYLIGHT_OFFSET_SEC, WAVESHARE_NTP_SERVER);
    ntpSyncStarted = true;
    Serial.println("[TIME] NTP sync requested");
}

static const char *weatherNetworkLabel() {
    if (WiFi.status() == WL_CONNECTED) return "在线";
    switch (weatherWifiState) {
        case WeatherWifiState::ConnectingPrimary:
        case WeatherWifiState::ConnectingSecondary:
            return "连接中";
        case WeatherWifiState::RetryLater:
            return "离线重试";
        default:
            return hasWeatherWifiConfig() ? "离线" : "未配置";
    }
}

static void applyCachedWeatherFromConfig() {
    if (!Config::getWeatherCacheValid()) return;
    uint8_t rawType = Config::getWeatherCacheType();
    WeatherData cached;
    cached.temperature = Config::getWeatherCacheTemperature();
    cached.humidity = companion.getHumidityPercent();
    cached.type = rawType <= static_cast<uint8_t>(WeatherType::UNKNOWN)
        ? static_cast<WeatherType>(rawType)
        : WeatherType::UNKNOWN;
    cached.isDay = Config::getWeatherCacheIsDay();
    cached.valid = true;
    if (!companion.isWeatherSimMode()) {
        companion.setWeather(cached);
    }
}

static void cacheWeatherData(const WeatherData& data) {
    if (!data.valid || data.type == WeatherType::UNKNOWN) return;
    uint8_t type = static_cast<uint8_t>(data.type);
    if (weatherCachedThisSession &&
        Config::getWeatherCacheValid() &&
        Config::getWeatherCacheType() == type &&
        fabsf(Config::getWeatherCacheTemperature() - data.temperature) < 0.2f &&
        Config::getWeatherCacheIsDay() == data.isDay) {
        return;
    }
    Config::setWeatherCache(true, data.temperature, type, data.isDay);
    Config::save();
    weatherCachedThisSession = true;
}

static void applyWeatherDataIfReady() {
    const WeatherData& data = weatherClient.getData();
    if (!data.valid) return;
    if (!companion.isWeatherSimMode()) {
        companion.setWeather(data);
    }
    cacheWeatherData(data);
}

static bool beginTownSync() {
    if (townSyncActive) {
        townSyncLeaseUntil = millis() + TOWN_SYNC_LEASE_MS;
        return true;
    }
    townSyncActive = true;
    townSyncLeaseUntil = millis() + TOWN_SYNC_LEASE_MS;
    companion.setTownSyncActive(true);
    companion.showNotification("Catputer", "进城中", "电脑端正在接走小猫");
    setActionMessage("小猫进城了");
    refreshUi();
    return true;
}

static bool renewTownSyncLease() {
    if (!townSyncActive) return false;
    townSyncLeaseUntil = millis() + TOWN_SYNC_LEASE_MS;
    return true;
}

static void endTownSync(const char *message = nullptr) {
    if (!townSyncActive) return;
    townSyncActive = false;
    townSyncLeaseUntil = 0;
    companion.setTownSyncActive(false);
    if (message && message[0]) {
        companion.showNotification("Catputer", "回家了", message);
        setActionMessage(message);
    }
    refreshUi();
}

static void expireTownSyncIfNeeded() {
    if (!townSyncActive) return;
    if (static_cast<long>(millis() - townSyncLeaseUntil) <= 0) return;
    endTownSync("连接断开，小猫自己回家了");
}

static String buildTownSyncSnapshotJson() {
    JsonDocument doc;
    doc["ok"] = true;
    JsonObject snapshot = doc["snapshot"].to<JsonObject>();

    JsonObject petObj = snapshot["pet"].to<JsonObject>();
    petObj["fullness"] = companion.getFullness();
    petObj["mood"] = companion.getMood();
    petObj["energy"] = companion.getEnergy();
    petObj["cleanliness"] = companion.getCleanliness();
    petObj["bond"] = companion.getBond();
    petObj["id"] = companion.getPetId();
    petObj["name"] = companion.getPetName();
    petObj["kind"] = companion.getPetKind();
    petObj["personality"] = companion.getPetPersonality();
    petObj["x"] = companion.getX();
    petObj["y"] = companion.getY();
    petObj["facing_left"] = companion.isFacingLeft();
    petObj["sleeping"] = companion.isSleeping();

    JsonObject deviceObj = snapshot["device"].to<JsonObject>();
    deviceObj["id"] = deviceId;
    deviceObj["type"] = DEVICE_TYPE;
    deviceObj["name"] = DEVICE_NAME;
    JsonObject capsObj = deviceObj["caps"].to<JsonObject>();
    capsObj["touch"] = DEVICE_CAP_TOUCH;
    capsObj["keyboard"] = DEVICE_CAP_KEYBOARD;
    capsObj["mic"] = DEVICE_CAP_MIC;
    capsObj["speaker"] = DEVICE_CAP_SPEAKER;

    JsonArray souvenirs = snapshot["souvenirs"].to<JsonArray>();
    for (uint8_t i = 0; i < companion.getSouvenirCount(); i++) {
        JsonObject item = souvenirs.add<JsonObject>();
        item["item"] = companion.getSouvenirItem(i);
        item["note"] = companion.getSouvenirNote(i);
    }

    snapshot["chat"].to<JsonArray>();

    int promptDays[PetStorage::PROMPT_SLOT_COUNT];
    char promptReplies[PetStorage::PROMPT_SLOT_COUNT][PetStorage::PROMPT_REPLY_LEN] = {{0}};
    companion.exportPromptMemory(promptDays, promptReplies);
    JsonObject memoryObj = snapshot["memory"].to<JsonObject>();
    JsonArray promptArr = memoryObj["prompts"].to<JsonArray>();
    static const char *labels[PetStorage::PROMPT_SLOT_COUNT] = {
        "早安", "早饭", "午饭", "晚饭", "晚睡", "心情"
    };
    for (uint8_t i = 0; i < PetStorage::PROMPT_SLOT_COUNT; i++) {
        if (!promptReplies[i][0]) continue;
        JsonObject row = promptArr.add<JsonObject>();
        row["slot"] = i;
        row["label"] = labels[i];
        row["day"] = promptDays[i];
        row["reply"] = promptReplies[i];
    }

    char followupLines[PetStorage::PROMPT_SLOT_COUNT][PetStorage::PROMPT_REPLY_LEN] = {{0}};
    uint8_t followupCount = 0;
    companion.exportPromptFollowups(followupLines, followupCount);
    JsonArray followupArr = memoryObj["followups"].to<JsonArray>();
    for (uint8_t i = 0; i < followupCount; i++) {
        if (followupLines[i][0]) followupArr.add(followupLines[i]);
    }

    char memoryLines[6][96] = {{0}};
    uint8_t memoryCount = 0;
    if (PetStorage::loadRecentPetMemoryEvents(companion.getPetId().c_str(), memoryLines, memoryCount, 6)) {
        JsonArray memoryEvents = memoryObj["events"].to<JsonArray>();
        for (uint8_t i = 0; i < memoryCount; i++) {
            memoryEvents.add(memoryLines[i]);
        }
    }

    String out;
    serializeJson(doc, out);
    return out;
}

static bool applyTownSyncSnapshotJson(const String& snapshotJson) {
    JsonDocument doc;
    if (deserializeJson(doc, snapshotJson) != DeserializationError::Ok) {
        Serial.println("[SYNC] Waveshare failed to parse desktop snapshot");
        return false;
    }

    JsonObject petObj = doc["pet"];
    if (petObj.isNull()) return false;

    JsonObject routeObj = doc["route"];
    if (!routeObj.isNull()) {
        const char *targetDeviceId = routeObj["target_device_id"] | "";
        if (targetDeviceId[0] && deviceId.length() > 0 && strcmp(targetDeviceId, deviceId.c_str()) != 0) {
            Serial.printf("[SYNC] Route target mismatch: %s != %s\n", targetDeviceId, deviceId.c_str());
        }
    }

    const char *status = petObj["status"] | "";
    companion.applySyncSnapshot(
        petObj["fullness"] | companion.getFullness(),
        petObj["mood"] | companion.getMood(),
        petObj["energy"] | companion.getEnergy(),
        petObj["cleanliness"] | companion.getCleanliness(),
        petObj["bond"] | companion.getBond(),
        petObj["x"] | companion.getX(),
        petObj["y"] | companion.getY(),
        petObj["facing_left"] | companion.isFacingLeft(),
        petObj["sleeping"] | companion.isSleeping(),
        status,
        petObj["id"] | companion.getPetId().c_str(),
        petObj["name"] | companion.getPetName().c_str(),
        petObj["kind"] | companion.getPetKind().c_str(),
        petObj["personality"] | companion.getPetPersonality().c_str()
    );

    char souvenirItems[PetStorage::MAX_SOUVENIRS][PetStorage::SOUVENIR_ITEM_LEN] = {{0}};
    char souvenirNotes[PetStorage::MAX_SOUVENIRS][PetStorage::SOUVENIR_NOTE_LEN] = {{0}};
    uint8_t souvenirCount = 0;
    JsonArray souvenirs = doc["souvenirs"].as<JsonArray>();
    for (JsonVariant value : souvenirs) {
        if (souvenirCount >= PetStorage::MAX_SOUVENIRS) break;
        const char *item = value["item"] | "";
        const char *note = value["note"] | "";
        strncpy(souvenirItems[souvenirCount], item, PetStorage::SOUVENIR_ITEM_LEN - 1);
        strncpy(souvenirNotes[souvenirCount], note, PetStorage::SOUVENIR_NOTE_LEN - 1);
        souvenirCount++;
    }
    companion.replaceSouvenirs(souvenirItems, souvenirNotes, souvenirCount);

    int promptDays[PetStorage::PROMPT_SLOT_COUNT];
    char promptReplies[PetStorage::PROMPT_SLOT_COUNT][PetStorage::PROMPT_REPLY_LEN] = {{0}};
    for (uint8_t i = 0; i < PetStorage::PROMPT_SLOT_COUNT; i++) {
        promptDays[i] = -1;
    }
    JsonObject memoryObj = doc["memory"];
    JsonArray promptArr = memoryObj["prompts"].as<JsonArray>();
    for (JsonVariant value : promptArr) {
        int slot = value["slot"] | -1;
        if (slot < 0 || slot >= PetStorage::PROMPT_SLOT_COUNT) continue;
        promptDays[slot] = value["day"] | -1;
        const char *reply = value["reply"] | "";
        strncpy(promptReplies[slot], reply, PetStorage::PROMPT_REPLY_LEN - 1);
        promptReplies[slot][PetStorage::PROMPT_REPLY_LEN - 1] = '\0';
    }
    companion.importPromptMemory(promptDays, promptReplies);
    endTownSync("从电脑回城了");
    return true;
}

static void startSyncNetworkServices() {
    if (syncNetworkServicesStarted || WiFi.status() != WL_CONNECTED) return;
    String target = Config::getGatewayHost();
    stateBroadcastBegin(target.c_str());
    cmdServer.begin();
    cmdServer.onSyncEnter([]() -> String {
        if (!townSyncActive) {
            beginTownSync();
        }
        renewTownSyncLease();
        return buildTownSyncSnapshotJson();
    });
    cmdServer.onSyncPing([]() -> bool {
        return renewTownSyncLease();
    });
    cmdServer.onSyncLeave([](const String& snapshotJson) -> bool {
        return applyTownSyncSnapshotJson(snapshotJson);
    });
    cmdServer.onNotify([](const char *app, const char *title, const char *body) {
        companion.showNotification(app, title, body);
    });
    syncNetworkServicesStarted = true;
    Serial.printf("[SYNC] Waveshare services ready at %s id=%s\n",
                  WiFi.localIP().toString().c_str(),
                  deviceId.c_str());
}

static void serviceWeatherNetwork() {
    // Touch AI request is memory-heavy (TLS + SSE). Pause weather polling while it is active
    // to avoid concurrent network pressure causing reboot on low-memory bursts.
    if (touchAiInFlight) return;
    ensureWeatherClientCity();

    if (WiFi.status() == WL_CONNECTED) {
        if (weatherWifiState != WeatherWifiState::Online) {
            weatherWifiState = WeatherWifiState::Online;
            Serial.printf("[WEATHER] WiFi connected: %s\n", WiFi.localIP().toString().c_str());
            setActionMessage("网络已连接");
        }
        ensureNtpSyncStarted();
        startSyncNetworkServices();
        unsigned long now = millis();
        if (now - weatherUpdateLastTickMs >= WEATHER_UPDATE_TICK_MS) {
            weatherClient.update();
            weatherUpdateLastTickMs = now;
        }
        applyWeatherDataIfReady();
        return;
    }

    unsigned long now = millis();
    if (!hasWeatherWifiConfig()) {
        weatherWifiState = WeatherWifiState::Idle;
        return;
    }

    switch (weatherWifiState) {
        case WeatherWifiState::Idle:
            if (now > 3000 && !startWeatherWifi(false)) {
                startWeatherWifi(true);
            }
            break;
        case WeatherWifiState::ConnectingPrimary:
            if (now - weatherWifiStartedMs > WEATHER_CONNECT_TIMEOUT_MS) {
                Serial.printf("[WEATHER] Primary WiFi timed out, status=%d\n", (int)WiFi.status());
                WiFi.disconnect(false, false);
                if (!startWeatherWifi(true)) scheduleWeatherWifiRetry(WEATHER_RETRY_DELAY_MS);
            }
            break;
        case WeatherWifiState::ConnectingSecondary:
            if (now - weatherWifiStartedMs > WEATHER_CONNECT_TIMEOUT_MS) {
                Serial.printf("[WEATHER] Secondary WiFi timed out, status=%d\n", (int)WiFi.status());
                WiFi.disconnect(false, false);
                scheduleWeatherWifiRetry(WEATHER_RETRY_DELAY_MS);
            }
            break;
        case WeatherWifiState::Online:
            syncNetworkServicesStarted = false;
            scheduleWeatherWifiRetry(WEATHER_RETRY_DELAY_MS);
            break;
        case WeatherWifiState::RetryLater:
            if (static_cast<long>(now - weatherWifiRetryAtMs) >= 0) {
                weatherWifiState = WeatherWifiState::Idle;
            }
            break;
    }
}

static bool ensureTtsBuffer() {
    if (ttsBuffer) return true;

    constexpr size_t kTargetSamples = 16000 * 5;
    constexpr size_t kFallbackSamples = 16000 * 2;
    size_t samples = kTargetSamples;
    size_t bytes = samples * sizeof(int16_t);
    Serial.printf("[TTS] PSRAM size=%u free=%u heap=%u largest8=%u largestPsram=%u\n",
                  ESP.getPsramSize(),
                  ESP.getFreePsram(),
                  ESP.getFreeHeap(),
                  heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                  heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (ESP.getPsramSize() > 0) {
        ttsBuffer = static_cast<int16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }
    if (!ttsBuffer) {
        samples = kFallbackSamples;
        bytes = samples * sizeof(int16_t);
        ttsBuffer = static_cast<int16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_8BIT));
    }
    if (!ttsBuffer && ESP.getPsramSize() > 0) {
        ttsBuffer = static_cast<int16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }
    if (ttsBuffer) {
        ttsMaxSamples = samples;
        Serial.printf("[TTS] Waveshare buffer allocated: %u bytes (%u samples)\n",
                      static_cast<unsigned>(bytes),
                      static_cast<unsigned>(ttsMaxSamples));
        return true;
    }

    ttsMaxSamples = 0;
    Serial.printf("[TTS] Waveshare buffer allocation failed, bytes=%u heap=%u freePsram=%u largest8=%u largestPsram=%u\n",
                  static_cast<unsigned>(bytes),
                  ESP.getFreeHeap(),
                  ESP.getFreePsram(),
                  heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                  heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    return false;
}

static void releaseTtsBufferIfIdle() {
    if (ttsPlayback.isPlaying() || companion.hasPendingSpeech()) return;
    if (!ttsBuffer) return;
    heap_caps_free(ttsBuffer);
    ttsBuffer = nullptr;
    ttsMaxSamples = 0;
    ttsPlayback.setBuffer(nullptr, 0);
    Serial.println("[TTS] Waveshare buffer released");
}

static void serviceLocalTts() {
    if (WAVESHARE_TTS_DISABLED) {
        releaseTtsBufferIfIdle();
        if (companion.hasPendingSpeech()) {
            char drop[128];
            int guard = 0;
            while (companion.hasPendingSpeech() && guard < 8) {
                if (!companion.takePendingSpeech(drop, sizeof(drop))) break;
                ++guard;
            }
        }
        return;
    }

    if (!Config::getAutoSpeak()) {
        if (companion.hasPendingSpeech() && !localTtsAutoSpeakWarned) {
            setActionMessage("TTS off");
            setTtsStatus("TTS off: auto speak disabled");
            localTtsAutoSpeakWarned = true;
        }
        releaseTtsBufferIfIdle();
        return;
    }
    localTtsAutoSpeakWarned = false;
    if (ttsPlayback.isPlaying()) return;
    if (!companion.hasPendingSpeech()) {
        releaseTtsBufferIfIdle();
        return;
    }
    if (!localTts.canSynthesize()) {
        if (!localTtsWarned) {
            Serial.printf("[TTS] Local TTS not ready: %s\n", localTts.availabilityReason());
            char msg[96];
            snprintf(msg, sizeof(msg), "TTS: %s", localTts.availabilityReason());
            setActionMessage(msg);
            setTtsStatus(msg, 10000);
            localTtsWarned = true;
        }
        return;
    }
    if (!ensureTtsBuffer()) {
        setTtsStatus("TTS: buffer allocation failed", 10000);
        return;
    }

    char speech[96];
    if (!companion.takePendingSpeech(speech, sizeof(speech))) return;
    ttsPlayback.setBuffer(ttsBuffer, ttsMaxSamples);
    if (!ttsPlayback.requestAndPlay(speech)) {
        Serial.printf("[TTS] Waveshare local speech failed: %s\n", speech);
        setActionMessage("TTS synth failed");
        setTtsStatus("TTS: synth failed", 10000);
    }
}

static void showMainActions(const char *message = nullptr) {
    showingMorePage = false;
    actionDrawerOpen = true;
    quickPanelOpen = false;
    settingsPanelOpen = false;
    if (message) setActionMessage(message);
    else setActionMessage("已回到主页动作");
    refreshUi();
}

static void showMoreActions(const char *message = nullptr) {
    showingMorePage = true;
    actionDrawerOpen = true;
    quickPanelOpen = false;
    settingsPanelOpen = false;
    if (message) setActionMessage(message);
    else setActionMessage("已切到更多动作");
    refreshUi();
}

static void toggleActionsFromPrimaryButton() {
    if (settingsPanelOpen) {
        settingsPanelOpen = false;
        quickPanelOpen = true;
        actionDrawerOpen = false;
        setActionMessage("已回到信息面板");
        refreshUi();
        return;
    }
    if (quickPanelOpen) {
        settingsPanelOpen = true;
        quickPanelOpen = false;
        actionDrawerOpen = false;
        setSettingsStatus("设置：请选择操作");
        setActionMessage("已打开设置界面");
        refreshUi();
        return;
    }
    quickPanelOpen = false;
    settingsPanelOpen = false;
    if (!actionDrawerOpen) {
        actionDrawerOpen = true;
        setActionMessage("PWR：打开动作抽屉");
    } else {
        showingMorePage = !showingMorePage;
        setActionMessage(showingMorePage ? "PWR：更多动作" : "PWR：主页动作");
    }
    refreshUi();
}

static void returnToHomeFromBackButton() {
    showingDetails = false;
    showingMorePage = false;
    actionDrawerOpen = false;
    quickPanelOpen = false;
    settingsPanelOpen = false;
    if (souvenirPanel) lv_obj_add_flag(souvenirPanel, LV_OBJ_FLAG_HIDDEN);
    souvenirPanelUntilMs = 0;
    setActionMessage("BOOT：返回主页");
    refreshUi();
}

static void handleRootGesture(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) return;
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (isSouvenirPanelVisible()) {
        if (dir == LV_DIR_LEFT) moveSouvenirPage(1);
        else if (dir == LV_DIR_RIGHT) moveSouvenirPage(-1);
        else if (dir == LV_DIR_TOP || dir == LV_DIR_BOTTOM) hideSouvenirPanel();
        lv_indev_wait_release(indev);
        return;
    }
    if (settingsPanelOpen && dir == LV_DIR_BOTTOM) {
        settingsPanelOpen = false;
        quickPanelOpen = true;
        setActionMessage("已回到信息面板");
        refreshUi();
        lv_indev_wait_release(indev);
        return;
    }
    if (dir == LV_DIR_LEFT) {
        showMoreActions("左滑：更多动作");
    } else if (dir == LV_DIR_RIGHT) {
        showMainActions("右滑：主页动作");
    } else if (dir == LV_DIR_TOP) {
        actionDrawerOpen = true;
        quickPanelOpen = false;
        settingsPanelOpen = false;
        setActionMessage(showingMorePage ? "上滑：打开更多动作" : "上滑：打开主页动作");
        refreshUi();
    } else if (dir == LV_DIR_BOTTOM) {
        actionDrawerOpen = false;
        settingsPanelOpen = false;
        quickPanelOpen = !quickPanelOpen;
        showingDetails = false;
        setActionMessage(quickPanelOpen ? "下滑：打开信息面板，PWR进设置" : "下滑：回到表盘");
        refreshUi();
    }
    lv_indev_wait_release(indev);
}

static void handleSwipe(PendingSwipe swipe) {
    if (isSouvenirPanelVisible()) {
        if (swipe == PendingSwipe::Left) moveSouvenirPage(1);
        else if (swipe == PendingSwipe::Right) moveSouvenirPage(-1);
        else if (swipe == PendingSwipe::Down || swipe == PendingSwipe::Up) hideSouvenirPanel();
        return;
    }
    if (settingsPanelOpen && swipe == PendingSwipe::Down) {
        settingsPanelOpen = false;
        quickPanelOpen = true;
        setActionMessage("已回到信息面板");
        refreshUi();
        return;
    }
    switch (swipe) {
        case PendingSwipe::Left:
            showMoreActions("左滑：更多动作");
            break;
        case PendingSwipe::Right:
            showMainActions("右滑：主页动作");
            break;
        case PendingSwipe::Up:
            actionDrawerOpen = true;
            quickPanelOpen = false;
            settingsPanelOpen = false;
            setActionMessage(showingMorePage ? "上滑：打开更多动作" : "上滑：打开主页动作");
            refreshUi();
            break;
        case PendingSwipe::Down:
            actionDrawerOpen = false;
            settingsPanelOpen = false;
            quickPanelOpen = !quickPanelOpen;
            showingDetails = false;
            setActionMessage(quickPanelOpen ? "下滑：打开信息面板，PWR进设置" : "下滑：回到表盘");
            refreshUi();
            break;
        case PendingSwipe::None:
            break;
    }
}

static void processPendingSwipe() {
    if (pendingSwipe == PendingSwipe::None) return;
    PendingSwipe swipe = pendingSwipe;
    pendingSwipe = PendingSwipe::None;
    handleSwipe(swipe);
}

static void pollHardwareButtons() {
    unsigned long now = millis();
    if (now - lastButtonPollMs < 20) return;
    lastButtonPollMs = now;

    bool bootNow = digitalRead(BOOT_BUTTON_PIN) == LOW;
    bool pwrNow = expander.digitalRead(PWR_BUTTON_EXIO);

    if (bootNow && !bootButtonPressed) {
        bootButtonDownMs = now;
    } else if (!bootNow && bootButtonPressed) {
        returnToHomeFromBackButton();
    }
    bootButtonPressed = bootNow;

    if (pwrNow && !pwrButtonPressed) {
        pwrButtonDownMs = now;
        pwrHoldAnnounced = false;
    } else if (pwrNow && !pwrHoldAnnounced && now - pwrButtonDownMs >= 500) {
        pwrHoldAnnounced = true;
        setActionMessage("PWR：按住说话，松开结束");
    } else if (!pwrNow && pwrButtonPressed) {
        unsigned long heldMs = now - pwrButtonDownMs;
        if (heldMs >= 500) {
            localTtsWarned = false;
            localTtsAutoSpeakWarned = false;
            actionDrawerOpen = false;
            quickPanelOpen = false;
            settingsPanelOpen = false;
            hideSouvenirPanel();
            setActionMessage("语音输入还在接入中");
            refreshUi();
        } else {
            toggleActionsFromPrimaryButton();
        }
    }
    pwrButtonPressed = pwrNow;
}

static void myDispFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, reinterpret_cast<uint16_t *>(&color_p->full), w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, reinterpret_cast<uint16_t *>(&color_p->full), w, h);
#endif
    lv_disp_flush_ready(disp);
}

static void myTouchRead(lv_indev_drv_t *, lv_indev_data_t *data) {
    int fingers = FT3168->IIC_Read_Device_Value(
        FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
    bool pressed = fingers > 0 || FT3168->IIC_Interrupt_Flag;
    if (pressed) {
        FT3168->IIC_Interrupt_Flag = false;
        int32_t touchX = FT3168->IIC_Read_Device_Value(
            FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
        int32_t touchY = FT3168->IIC_Read_Device_Value(
            FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
        if (touchX >= 0 && touchY >= 0) {
            if (touchX >= LCD_WIDTH || touchY >= LCD_HEIGHT) {
                if (touchX < LCD_HEIGHT && touchY < LCD_WIDTH) {
                    int32_t sx = touchX;
                    touchX = touchY;
                    touchY = sx;
                }
            }
            if (touchX < 0) touchX = 0;
            if (touchY < 0) touchY = 0;
            if (touchX >= LCD_WIDTH) touchX = LCD_WIDTH - 1;
            if (touchY >= LCD_HEIGHT) touchY = LCD_HEIGHT - 1;
            lastTouchPoint.x = touchX;
            lastTouchPoint.y = touchY;
        }
        data->point = lastTouchPoint;
        data->state = LV_INDEV_STATE_PR;
        if (!touchTracking) {
            touchTracking = true;
            touchStartPoint = lastTouchPoint;
            touchStartMs = millis();
        }
    } else {
        data->point = lastTouchPoint;
        data->state = LV_INDEV_STATE_REL;
        if (touchTracking) {
            int dx = static_cast<int>(lastTouchPoint.x) - static_cast<int>(touchStartPoint.x);
            int dy = static_cast<int>(lastTouchPoint.y) - static_cast<int>(touchStartPoint.y);
            unsigned long elapsed = millis() - touchStartMs;
            const int threshold = 56;
            if (elapsed <= 1200) {
                if (abs(dx) > abs(dy) && abs(dx) >= threshold) {
                    pendingSwipe = dx < 0 ? PendingSwipe::Left : PendingSwipe::Right;
                } else if (abs(dy) >= threshold) {
                    pendingSwipe = dy < 0 ? PendingSwipe::Up : PendingSwipe::Down;
                }
            }
            touchTracking = false;
        }
    }
}

static void lvTick(void *) {
    lv_tick_inc(2);
}

static void updateActionButtons() {
    if (companion.hasActivePrompt()) {
        static const char *fallbackChoices[3] = {"1 好呀", "2 等等", "3 不知道"};
        const char *choices[3] = {
            companion.getActivePromptChoice(0),
            companion.getActivePromptChoice(1),
            companion.getActivePromptChoice(2)
        };
        const HomeAction promptActions[4] = {
            HomeAction::PromptChoice1,
            HomeAction::PromptChoice2,
            HomeAction::PromptChoice3,
            HomeAction::PromptDismiss
        };
        for (int i = 0; i < 4; ++i) {
            const char *label = (i < 3 && choices[i] && choices[i][0]) ? choices[i] : (i < 3 ? fallbackChoices[i] : "稍后");
            lv_label_set_text(actionButtonLabels[i], label);
            currentButtonActions[i] = promptActions[i];
        }
        return;
    }

    const ActionSlot *slots = showingMorePage ? pageMore : pageMain;
    for (int i = 0; i < 4; ++i) {
        lv_label_set_text(actionButtonLabels[i], slots[i].label);
        currentButtonActions[i] = slots[i].action;
    }
}

static void updateOverlayPanels() {
    if (actionPanel) {
        lv_obj_clear_flag(actionPanel, LV_OBJ_FLAG_HIDDEN);
    }
    if (actionButtonWrap) {
        if (actionDrawerOpen) lv_obj_clear_flag(actionButtonWrap, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(actionButtonWrap, LV_OBJ_FLAG_HIDDEN);
    }
    if (quickPanel) {
        if (quickPanelOpen) lv_obj_clear_flag(quickPanel, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(quickPanel, LV_OBJ_FLAG_HIDDEN);
    }
    if (settingsPanel) {
        if (settingsPanelOpen) {
            lv_obj_clear_flag(settingsPanel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(settingsPanel);
        } else {
            lv_obj_add_flag(settingsPanel, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (statsCard) {
        lv_obj_add_flag(statsCard, LV_OBJ_FLAG_HIDDEN);
    }
    if (ttsStatusPanel && ttsStatusUntilMs != 0 && millis() > ttsStatusUntilMs) {
        lv_obj_add_flag(ttsStatusPanel, LV_OBJ_FLAG_HIDDEN);
        ttsStatusUntilMs = 0;
    }
    if (quickPanelLabel) {
        uint8_t volume = safeVolume(Config::getSpeakerVolume());
        uint64_t sdSize = PetStorage::cardSizeMB();
        int provisionLeft = provisionSecondsLeft();
        char provisionExtra[32];
        auto ota = SdOta::snapshot();
        char otaLine[64];
        switch (ota.state) {
            case SdOta::State::Running:
                snprintf(otaLine, sizeof(otaLine), "刷机中 %d%%", ota.progressPercent);
                break;
            case SdOta::State::Success:
                snprintf(otaLine, sizeof(otaLine), "刷机完成，重启生效");
                break;
            case SdOta::State::Failed:
                snprintf(otaLine, sizeof(otaLine), "刷机失败");
                break;
            default:
                snprintf(otaLine, sizeof(otaLine), "待机（点下中开始）");
                break;
        }
        String access = provisionIsActive() || provisionCtx.state == ProvisionState::Success
                            ? provisionAccessAddress()
                            : String("-");
        if (provisionIsActive()) {
            snprintf(provisionExtra, sizeof(provisionExtra), " 剩余%ds", provisionLeft);
        } else {
            provisionExtra[0] = '\0';
        }
        lv_label_set_text_fmt(
            quickPanelLabel,
            "亲密%d  纪念%d  音量%u/60\n天气 %s %dC  网络 %s\nSD %s %lluMB  配网 %s%s\n访问 %s\n刷机 %s",
            companion.getBond(),
            companion.getSouvenirCount(),
            volume,
            weatherName(companion.getWeatherType()),
            roundedTemperature(),
            weatherNetworkLabel(),
            sdReady ? "OK" : "N/A",
            sdReady ? sdSize : 0,
            provisionStateLabel(),
            provisionExtra,
            access.c_str(),
            otaLine);
#if 0
        lv_label_set_text_fmt(
            quickPanelLabel,
            "快捷面板\n亮度 %d\n语音 PWR 长按\n返回 BOOT\n触摸 %d,%d",
            220,
            static_cast<int>(lastTouchPoint.x),
            static_cast<int>(lastTouchPoint.y));
#endif
    }
    if (quickProvisionBtnLabel) {
        lv_label_set_text(quickProvisionBtnLabel, provisionIsActive() ? "取消配网" : (PROVISION_BLE_ENABLED ? "蓝牙配网" : "开启配网"));
    }
    if (settingsStatusLabel && settingsPanelOpen) {
        auto ota = SdOta::snapshot();
        if (ota.state == SdOta::State::Running) {
            lv_label_set_text_fmt(settingsStatusLabel, "SD刷机中 %d%%", ota.progressPercent);
        } else if (ota.state == SdOta::State::Failed || ota.state == SdOta::State::Success) {
            lv_label_set_text(settingsStatusLabel, ota.message.c_str());
        } else if (provisionIsActive()) {
            lv_label_set_text_fmt(settingsStatusLabel, "配网 %s，剩余 %ds\n%s",
                                  provisionStateLabel(),
                                  provisionSecondsLeft(),
                                  provisionAccessAddress().c_str());
        }
    }
    int quickValues[4] = {
        companion.getFullness(),
        companion.getMood(),
        companion.getEnergy(),
        companion.getCleanliness()};
    for (int i = 0; i < 4; ++i) {
        if (quickPanelStatValues[i]) lv_label_set_text_fmt(quickPanelStatValues[i], "%d/100", quickValues[i]);
    }
}

static void updateDetailsPanel() {
    if (!detailsPanel || !detailsLabel) return;
    if (showingDetails) lv_obj_clear_flag(detailsPanel, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(detailsPanel, LV_OBJ_FLAG_HIDDEN);
    if (!showingDetails) return;
    lv_label_set_text_fmt(
        detailsLabel,
        "亲密 %d\n品种 %s\n天气 %s\n温度 %dC\n纪念 %d",
        companion.getBond(),
        companion.getPetKind() == "purple" ? "紫猫" : "橙猫",
        weatherName(companion.getWeatherType()),
        roundedTemperature(),
        companion.getSouvenirCount());
    if (companion.getPetKind() == "q") {
        lv_label_set_text_fmt(
            detailsLabel,
            "亲密 %d\n品种 %s\n天气 %s\n温度 %dC\n纪念 %d",
            companion.getBond(),
            petKindLabel(),
            weatherName(companion.getWeatherType()),
            roundedTemperature(),
            companion.getSouvenirCount());
    }
}

static void hideSouvenirPanel() {
    if (souvenirPanel) lv_obj_add_flag(souvenirPanel, LV_OBJ_FLAG_HIDDEN);
    souvenirPanelUntilMs = 0;
}

static uint8_t firstPhotoSouvenirIndex() {
    uint8_t count = companion.getSouvenirCount();
    for (uint8_t i = 0; i < count; ++i) {
        if (isPhotoSouvenir(companion.getSouvenirItem(i))) return i;
    }
    return 0;
}

static void renderSouvenirPanel() {
    uint8_t count = companion.getSouvenirCount();
    if (count == 0 || !souvenirPanel || !souvenirTitleLabel || !souvenirItemLabel || !souvenirNoteLabel) return;
    if (souvenirPanelIndex >= count) souvenirPanelIndex = 0;

    const char *item = companion.getSouvenirItem(souvenirPanelIndex);
    const char *note = companion.getSouvenirNote(souvenirPanelIndex);
    bool photo = isPhotoSouvenir(item);
    const char *key = photoKey(item);
    const lv_img_dsc_t *photoImage = photo ? photoImageForKey(key) : nullptr;

    lv_label_set_text(souvenirTitleLabel, photo ? "照片纪念" : "纪念盒");
    if (souvenirIndexLabel) {
        lv_label_set_text_fmt(souvenirIndexLabel, "%u/%u", souvenirPanelIndex + 1, count);
    }

    if (souvenirPhotoFrame) {
        if (photo) lv_obj_clear_flag(souvenirPhotoFrame, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(souvenirPhotoFrame, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(souvenirPhotoFrame, photoColor(key), 0);
    }
    if (souvenirPhotoImage) {
        if (photoImage) {
            lv_img_set_src(souvenirPhotoImage, photoImage);
            lv_obj_clear_flag(souvenirPhotoImage, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(souvenirPhotoImage, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (souvenirPhotoLabel) {
        lv_label_set_text(souvenirPhotoLabel, photoImage ? "" : "照片");
        if (photoImage) lv_obj_add_flag(souvenirPhotoLabel, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(souvenirPhotoLabel, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(souvenirItemLabel, photo ? photoLabel(key) : ((item && item[0]) ? item : "神秘小物"));
    lv_obj_set_width(souvenirItemLabel, photo ? 180 : 284);
    lv_obj_align(souvenirItemLabel, LV_ALIGN_TOP_LEFT, photo ? 112 : 18, photo ? 48 : 44);

    String noteText = (note && note[0]) ? String(note) : String(photo ? "把风景带回来" : "这次外出留下了一点回忆");
    noteText.replace("|", "\n");
    lv_label_set_text(souvenirNoteLabel, noteText.c_str());
    lv_obj_set_width(souvenirNoteLabel, photo ? 180 : 284);
    lv_obj_align(souvenirNoteLabel, LV_ALIGN_TOP_LEFT, photo ? 112 : 18, photo ? 76 : 76);

    if (souvenirHintLabel) {
        lv_label_set_text(souvenirHintLabel, count > 1 ? "左右滑/点按翻页 · BOOT收起" : "点按/BOOT收起");
    }
}

static void showSouvenirPanel(uint8_t index = 0) {
    if (!souvenirPanel || companion.getSouvenirCount() == 0) return;
    actionDrawerOpen = false;
    quickPanelOpen = false;
    settingsPanelOpen = false;
    showingDetails = false;
    souvenirPanelIndex = index < companion.getSouvenirCount() ? index : 0;
    renderSouvenirPanel();

    lv_obj_clear_flag(souvenirPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(souvenirPanel);
    souvenirPanelUntilMs = millis() + 7000;
}

static bool isSouvenirPanelVisible() {
    return souvenirPanel && !lv_obj_has_flag(souvenirPanel, LV_OBJ_FLAG_HIDDEN);
}

static void moveSouvenirPage(int delta) {
    uint8_t count = companion.getSouvenirCount();
    if (count == 0) {
        hideSouvenirPanel();
        return;
    }
    int next = static_cast<int>(souvenirPanelIndex) + delta;
    while (next < 0) next += count;
    souvenirPanelIndex = static_cast<uint8_t>(next % count);
    renderSouvenirPanel();
    souvenirPanelUntilMs = millis() + 9000;
}

static void onSouvenirPanel(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    if (companion.getSouvenirCount() > 1) moveSouvenirPage(1);
    else hideSouvenirPanel();
}

static void updateSouvenirPanel() {
    if (souvenirPanelUntilMs != 0 && millis() > souvenirPanelUntilMs) hideSouvenirPanel();
}

static void updateOutingReturn() {
    bool outingNow = companion.isOuting();
    uint8_t souvenirCount = companion.getSouvenirCount();
    if (!outingNow && souvenirCount > 0 && (wasOuting || souvenirCount > lastSouvenirCount)) {
        showSouvenirPanel(0);
        setActionMessage("外出回来了");
    }
    wasOuting = outingNow;
    lastSouvenirCount = souvenirCount;
}

static void refreshUi() {
    updateHomeBackgroundFromSd();
    lv_color_t bgColor = weatherBgColor(companion.getWeatherType());
    lv_obj_set_style_bg_color(lv_scr_act(), bgColor, 0);
    lv_obj_set_style_bg_grad_color(lv_scr_act(), lv_color_hex(0x0d1824), 0);
    lv_obj_set_style_bg_grad_dir(lv_scr_act(), LV_GRAD_DIR_VER, 0);
    lv_label_set_text(nameLabel, companion.getPetName().c_str());
    lv_label_set_text(stateLabel, primaryStatusText());
    lv_label_set_text_fmt(weatherChipLabel, "%s %s %dC %s",
                          weatherCityLabel(),
                          weatherName(companion.getWeatherType()),
                          roundedTemperature(),
                          sdHomeBgLoaded ? "S" : "R");
    if (timeChipLabel) lv_label_set_text(timeChipLabel, currentTimeLabel());
    lv_label_set_text_fmt(bondChipLabel, "亲密 %d", companion.getBond());
    if (detailLabel) {
        bool showPersistentDetail = companion.hasActivePrompt() || companion.isTownSyncActive() || companion.isOuting();
        if (showPersistentDetail) {
            clearActionDetailPaging();
            lv_label_set_text(detailLabel, detailStatusText());
            lv_obj_clear_flag(detailLabel, LV_OBJ_FLAG_HIDDEN);
        } else if (actionMessageUntilMs != 0) {
            lv_obj_clear_flag(detailLabel, LV_OBJ_FLAG_HIDDEN);
            updateActionDetailPageIfNeeded();
        } else {
            lv_obj_add_flag(detailLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    int values[4] = {
        companion.getFullness(),
        companion.getMood(),
        companion.getEnergy(),
        companion.getCleanliness()};
    for (int i = 0; i < 4; ++i) {
        if (statValues[i]) lv_label_set_text_fmt(statValues[i], "%d/100", values[i]);
    }

    lv_point_t wander = updateRoomOffset();
    int bob = (companion.getFrameIndex() % 2 == 0) ? 0 : uiScaleY(3);
    lv_obj_align(petCard, LV_ALIGN_TOP_MID, wander.x, uiPetCardY() + wander.y + bob);
    lv_obj_invalidate(petCard);
    if (companion.hasActivePrompt()) {
        actionDrawerOpen = true;
        quickPanelOpen = false;
        settingsPanelOpen = false;
    }
    if (actionMessageUntilMs != 0 && millis() > actionMessageUntilMs) {
        lv_label_set_text(actionLabel, defaultActionMessage());
        actionMessageUntilMs = 0;
        clearActionDetailPaging();
        if (detailLabel && !(companion.hasActivePrompt() || companion.isTownSyncActive() || companion.isOuting())) {
            lv_obj_add_flag(detailLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (actionMessageUntilMs == 0) {
        lv_label_set_text(actionLabel, defaultActionMessage());
    }
    updateActionButtons();
    updateOverlayPanels();
    updateDetailsPanel();
    updateSouvenirPanel();
}

static void onAction(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    lv_obj_t *target = lv_event_get_target(e);
    HomeAction action = static_cast<HomeAction>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    for (int i = 0; i < 4; ++i) {
        if (target == actionButtons[i]) {
            action = currentButtonActions[i];
            break;
        }
    }
    switch (action) {
        case HomeAction::PromptChoice1:
        case HomeAction::PromptChoice2:
        case HomeAction::PromptChoice3: {
            uint8_t choiceIndex = action == HomeAction::PromptChoice1 ? 0 : (action == HomeAction::PromptChoice2 ? 1 : 2);
            if (companion.answerActivePromptChoice(choiceIndex)) {
                actionDrawerOpen = false;
                quickPanelOpen = false;
                settingsPanelOpen = false;
                setActionMessage("我记住啦");
            }
            break;
        }
        case HomeAction::PromptDismiss:
            companion.dismissActivePrompt();
            actionDrawerOpen = false;
            quickPanelOpen = false;
            settingsPanelOpen = false;
            setActionMessage("晚点再聊");
            break;
        case HomeAction::Feed:
            closeActionDrawerAfterRoomAction(action);
            setActionMessage("去吃点东西");
            break;
        case HomeAction::Play:
            closeActionDrawerAfterRoomAction(action);
            setActionMessage("去玩具旁边");
            break;
        case HomeAction::Clean:
            closeActionDrawerAfterRoomAction(action);
            setActionMessage("去洗干净");
            break;
        case HomeAction::Nap:
            closeActionDrawerAfterRoomAction(action);
            setActionMessage("去窝里躺会");
            break;
        case HomeAction::Out:
            companion.startOuting();
            wasOuting = companion.isOuting();
            setActionMessage(wasOuting ? "出门逛逛，回来会带纪念品" : "现在还不能外出");
            break;
        case HomeAction::Bag:
            if (companion.getSouvenirCount() > 0) {
                showSouvenirPanel(firstPhotoSouvenirIndex());
                setActionMessage("打开纪念盒");
            } else {
                setActionMessage("还没有带回纪念品");
            }
            break;
        case HomeAction::Stats:
            showingDetails = !showingDetails;
            quickPanelOpen = false;
            settingsPanelOpen = false;
            setActionMessage(showingDetails ? "已展开状态面板" : "已收起状态面板");
            break;
        case HomeAction::Happy:
        case HomeAction::PetTap:
            companion.triggerHappy();
            companion.noteUserAttention();
            // Touch interactions use text bubbles; avoid local TTS synth path
            // here because it can reboot on this hardware profile.
            discardPendingSpeech();
            if (triggerTouchCompanionAi()) {
                setActionMessage("让我想两句悄悄话");
            } else {
                String fallback = fallbackTouchBubble();
                setActionMessage(fallback.c_str());
                companion.speak(fallback.c_str());
                actionMessageUntilMs = millis() + 3600;
            }
            break;
        case HomeAction::TogglePage:
            showingMorePage = !showingMorePage;
            setActionMessage(showingMorePage ? "切到更多动作" : "切到主页动作");
            break;
    }
    refreshUi();
}

static void onQuickPanel(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    if (!quickPanelOpen) return;
    (void)e;
}

static void onQuickPanelAction(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_RELEASED) return;
    if (!settingsPanelOpen) return;
    uintptr_t action = reinterpret_cast<uintptr_t>(lv_event_get_user_data(e));
    switch (action) {
        case 0:
            adjustVolume(-5);
            setSettingsStatus("音量已降低");
            return;
        case 1:
            adjustVolume(5);
            setSettingsStatus("音量已提高");
            return;
        case 2:
            if (provisionIsActive()) {
                setSettingsStatus("正在取消配网...");
                cancelProvisioningSession();
            } else {
                setSettingsStatus("正在开启配网...");
                startProvisioningSession();
            }
            break;
        case 3:
            setSettingsStatus("正在重试配网...");
            retryProvisioningSession();
            break;
        case 4:
            setSettingsStatus("正在检查SD固件...");
            triggerSdOtaFromQuickPanel();
            break;
        default:
            break;
    }
    refreshUi();
}

static lv_obj_t *makeButton(lv_obj_t *parent, const char *text, HomeAction action) {
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_style_radius(button, 16, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x426891), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x5f8fc2), LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x7aa3d1), LV_STATE_FOCUSED);
    lv_obj_set_style_shadow_width(button, 10, 0);
    lv_obj_set_style_shadow_color(button, lv_color_hex(0x102030), 0);
    lv_obj_set_style_border_width(button, 2, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x8bb8e4), 0);
    lv_obj_add_event_cb(button, onAction, LV_EVENT_RELEASED, reinterpret_cast<void *>(static_cast<uintptr_t>(action)));
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, zhFont, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_center(label);
    return button;
}

static void buildUi() {
    const int contentW = uiContentWidth();
    const int petW = min(contentW, max(220, uiScaleX(240)));
    const int petH = max(136, uiScaleY(150));
    const int statsH = max(58, uiScaleY(66));
    const int actionPanelH = max(40, uiScaleY(42));
    const int detailW = min(344, max(260, LCD_WIDTH - 24));

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x18314a), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(lv_scr_act(), zhFont, 0);
    lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0xf4f7fb), 0);
    lv_obj_set_style_bg_grad_color(lv_scr_act(), lv_color_hex(0x0d1824), 0);
    lv_obj_set_style_bg_grad_dir(lv_scr_act(), LV_GRAD_DIR_VER, 0);
    lv_obj_add_flag(lv_scr_act(), LV_OBJ_FLAG_GESTURE_BUBBLE);

    stageLayer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(stageLayer, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_align(stageLayer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(stageLayer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stageLayer, 0, 0);
    lv_obj_set_style_pad_all(stageLayer, 0, 0);
    lv_obj_clear_flag(stageLayer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(stageLayer, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_move_background(stageLayer);
    lv_obj_add_event_cb(stageLayer, drawStageBackground, LV_EVENT_DRAW_MAIN, nullptr);

    nameLabel = lv_label_create(lv_scr_act());
    lv_obj_align(nameLabel, LV_ALIGN_TOP_LEFT, 20, 12);
    lv_obj_set_style_text_font(nameLabel, zhFont, 0);
    lv_obj_set_style_text_color(nameLabel, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_color(nameLabel, lv_color_hex(0x162536), 0);
    lv_obj_set_style_bg_opa(nameLabel, LV_OPA_80, 0);
    lv_obj_set_style_radius(nameLabel, 10, 0);
    lv_obj_set_style_pad_left(nameLabel, 8, 0);
    lv_obj_set_style_pad_right(nameLabel, 8, 0);
    lv_obj_set_style_pad_top(nameLabel, 3, 0);
    lv_obj_set_style_pad_bottom(nameLabel, 3, 0);

    weatherChip = lv_obj_create(lv_scr_act());
    lv_obj_set_size(weatherChip, min(160, max(136, LCD_WIDTH / 2 - 24)), 28);
    lv_obj_align(weatherChip, LV_ALIGN_TOP_RIGHT, -112, uiScaleY(10));
    lv_obj_set_style_radius(weatherChip, 14, 0);
    lv_obj_set_style_bg_color(weatherChip, lv_color_hex(0x2b4b6a), 0);
    lv_obj_set_style_border_width(weatherChip, 0, 0);
    lv_obj_clear_flag(weatherChip, LV_OBJ_FLAG_SCROLLABLE);
    weatherChipLabel = lv_label_create(weatherChip);
    lv_obj_set_style_text_color(weatherChipLabel, lv_color_hex(0xf7fbff), 0);
    lv_obj_set_style_text_font(weatherChipLabel, zhFont, 0);
    lv_obj_center(weatherChipLabel);

    timeChip = lv_obj_create(lv_scr_act());
    lv_obj_set_size(timeChip, min(160, max(136, LCD_WIDTH / 2 - 24)), 24);
    lv_obj_align_to(timeChip, weatherChip, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
    lv_obj_set_style_radius(timeChip, 12, 0);
    lv_obj_set_style_bg_color(timeChip, lv_color_hex(0x162536), 0);
    lv_obj_set_style_bg_opa(timeChip, LV_OPA_80, 0);
    lv_obj_set_style_border_width(timeChip, 0, 0);
    lv_obj_clear_flag(timeChip, LV_OBJ_FLAG_SCROLLABLE);
    timeChipLabel = lv_label_create(timeChip);
    lv_obj_set_style_text_color(timeChipLabel, lv_color_hex(0xf7fbff), 0);
    lv_obj_set_style_text_font(timeChipLabel, zhFont, 0);
    lv_obj_center(timeChipLabel);

    bondChip = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bondChip, 92, 28);
    lv_obj_align(bondChip, LV_ALIGN_TOP_RIGHT, -14, uiScaleY(10));
    lv_obj_set_style_radius(bondChip, 14, 0);
    lv_obj_set_style_bg_color(bondChip, lv_color_hex(0x4f3f6d), 0);
    lv_obj_set_style_border_width(bondChip, 0, 0);
    lv_obj_clear_flag(bondChip, LV_OBJ_FLAG_SCROLLABLE);
    bondChipLabel = lv_label_create(bondChip);
    lv_obj_set_style_text_color(bondChipLabel, lv_color_hex(0xfff3ff), 0);
    lv_obj_set_style_text_font(bondChipLabel, zhFont, 0);
    lv_obj_center(bondChipLabel);

    petCard = lv_obj_create(lv_scr_act());
    lv_obj_set_size(petCard, petW, petH);
    lv_obj_align(petCard, LV_ALIGN_TOP_MID, 0, uiPetCardY());
    lv_obj_set_style_radius(petCard, 28, 0);
    lv_obj_set_style_bg_opa(petCard, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(petCard, 0, 0);
    lv_obj_set_style_border_width(petCard, 0, 0);
    lv_obj_clear_flag(petCard, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(petCard, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(petCard, onAction, LV_EVENT_RELEASED, reinterpret_cast<void *>(static_cast<uintptr_t>(HomeAction::PetTap)));
    lv_obj_add_event_cb(petCard, drawPetSprite, LV_EVENT_DRAW_MAIN, nullptr);

    stateLabel = lv_label_create(lv_scr_act());
    lv_obj_align(stateLabel, LV_ALIGN_TOP_LEFT, 20, uiScaleY(42));
    lv_obj_set_style_text_font(stateLabel, zhFont, 0);
    lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xfff0c2), 0);
    lv_obj_set_style_bg_color(stateLabel, lv_color_hex(0x162536), 0);
    lv_obj_set_style_bg_opa(stateLabel, LV_OPA_80, 0);
    lv_obj_set_style_radius(stateLabel, 10, 0);
    lv_obj_set_style_pad_left(stateLabel, 8, 0);
    lv_obj_set_style_pad_right(stateLabel, 8, 0);
    lv_obj_set_style_pad_top(stateLabel, 3, 0);
    lv_obj_set_style_pad_bottom(stateLabel, 3, 0);

    detailLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(detailLabel, detailW);
    lv_label_set_long_mode(detailLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(detailLabel, LV_ALIGN_TOP_MID, 0, uiScaleY(258));
    lv_obj_set_style_text_font(detailLabel, zhFont, 0);
    lv_obj_set_style_text_color(detailLabel, lv_color_hex(0xe2ebf5), 0);
    lv_obj_set_style_text_align(detailLabel, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_bg_color(detailLabel, lv_color_hex(0x162536), 0);
    lv_obj_set_style_bg_opa(detailLabel, LV_OPA_70, 0);
    lv_obj_set_style_radius(detailLabel, 14, 0);
    lv_obj_set_style_pad_left(detailLabel, 10, 0);
    lv_obj_set_style_pad_right(detailLabel, 10, 0);
    lv_obj_set_style_pad_top(detailLabel, 5, 0);
    lv_obj_set_style_pad_bottom(detailLabel, 5, 0);
    lv_obj_add_flag(detailLabel, LV_OBJ_FLAG_HIDDEN);

    statsCard = lv_obj_create(lv_scr_act());
    lv_obj_set_size(statsCard, contentW, statsH);
    lv_obj_align(statsCard, LV_ALIGN_TOP_MID, 0, uiScaleY(298));
    lv_obj_set_style_radius(statsCard, 18, 0);
    lv_obj_set_style_bg_color(statsCard, lv_color_hex(0x162536), 0);
    lv_obj_set_style_bg_opa(statsCard, LV_OPA_30, 0);
    lv_obj_set_style_border_width(statsCard, 1, 0);
    lv_obj_set_style_border_color(statsCard, lv_color_hex(0x7ea6c8), 0);
    lv_obj_clear_flag(statsCard, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(statsCard, LV_OBJ_FLAG_GESTURE_BUBBLE);

    static const char *statNames[4] = {"饱腹", "心情", "精力", "清洁"};
    static const uint32_t statColors[4] = {0xffc35c, 0xff7db4, 0x7bd7ff, 0x8dffb2};
    const int statColW = (contentW - 36) / 2;
    for (int i = 0; i < 4; ++i) {
        int col = i % 2;
        int row = i / 2;
        lv_obj_t *label = lv_label_create(statsCard);
        lv_label_set_text(label, statNames[i]);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 18 + col * statColW, 4 + row * 26);
        lv_obj_set_style_text_font(label, zhFont, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xf7fbff), 0);

        statValues[i] = lv_label_create(statsCard);
        lv_obj_align(statValues[i], LV_ALIGN_TOP_LEFT, 68 + col * statColW, 4 + row * 26);
        lv_obj_set_style_text_font(statValues[i], statNumberFont, 0);
        lv_obj_set_style_text_color(statValues[i], lv_color_hex(statColors[i]), 0);
    }
    lv_obj_add_flag(statsCard, LV_OBJ_FLAG_HIDDEN);

    detailsPanel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(detailsPanel, min(170, max(140, LCD_WIDTH / 2)), max(112, uiScaleY(118)));
    lv_obj_align(detailsPanel, LV_ALIGN_TOP_RIGHT, -14, uiScaleY(92));
    lv_obj_set_style_radius(detailsPanel, 18, 0);
    lv_obj_set_style_bg_color(detailsPanel, lv_color_hex(0x2d4661), 0);
    lv_obj_set_style_border_width(detailsPanel, 0, 0);
    lv_obj_clear_flag(detailsPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(detailsPanel, LV_OBJ_FLAG_GESTURE_BUBBLE);
    detailsLabel = lv_label_create(detailsPanel);
    lv_obj_set_width(detailsLabel, min(146, max(116, LCD_WIDTH / 2 - 24)));
    lv_label_set_long_mode(detailsLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(detailsLabel, LV_ALIGN_TOP_LEFT, 12, 12);
    lv_obj_set_style_text_font(detailsLabel, zhFont, 0);
    lv_obj_set_style_text_color(detailsLabel, lv_color_hex(0xf7fbff), 0);
    lv_obj_add_flag(detailsPanel, LV_OBJ_FLAG_HIDDEN);

    actionPanel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(actionPanel, contentW, actionPanelH);
    lv_obj_align(actionPanel, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_radius(actionPanel, 18, 0);
    lv_obj_set_style_bg_color(actionPanel, lv_color_hex(0x213246), 0);
    lv_obj_set_style_bg_opa(actionPanel, LV_OPA_70, 0);
    lv_obj_set_style_border_width(actionPanel, 1, 0);
    lv_obj_set_style_border_color(actionPanel, lv_color_hex(0x7998b5), 0);
    lv_obj_clear_flag(actionPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(actionPanel, LV_OBJ_FLAG_GESTURE_BUBBLE);

    actionLabel = lv_label_create(actionPanel);
    lv_obj_set_width(actionLabel, contentW - 14);
    lv_label_set_long_mode(actionLabel, LV_LABEL_LONG_WRAP);
    lv_obj_center(actionLabel);
    lv_obj_set_style_text_font(actionLabel, zhFont, 0);
    lv_obj_set_style_text_color(actionLabel, lv_color_hex(0xfdf6d8), 0);
    lv_label_set_text(actionLabel, defaultActionMessage());

    actionButtonWrap = lv_obj_create(lv_scr_act());
    lv_obj_set_size(actionButtonWrap, contentW, max(100, uiScaleY(112)));
    lv_obj_align(actionButtonWrap, LV_ALIGN_CENTER, 0, uiScaleY(66));
    lv_obj_set_style_radius(actionButtonWrap, 24, 0);
    lv_obj_set_style_bg_color(actionButtonWrap, lv_color_hex(0x12243a), 0);
    lv_obj_set_style_bg_opa(actionButtonWrap, LV_OPA_80, 0);
    lv_obj_set_style_border_width(actionButtonWrap, 1, 0);
    lv_obj_set_style_border_color(actionButtonWrap, lv_color_hex(0x8bb8e4), 0);
    lv_obj_set_flex_flow(actionButtonWrap, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(actionButtonWrap, 10, 0);
    lv_obj_set_style_pad_column(actionButtonWrap, 10, 0);
    lv_obj_set_style_pad_all(actionButtonWrap, 12, 0);
    lv_obj_clear_flag(actionButtonWrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(actionButtonWrap, LV_OBJ_FLAG_GESTURE_BUBBLE);

    for (int i = 0; i < 4; ++i) {
        actionButtons[i] = makeButton(actionButtonWrap, pageMain[i].label, pageMain[i].action);
        lv_obj_set_size(actionButtons[i], (contentW - 46) / 2, 38);
        lv_obj_add_flag(actionButtons[i], LV_OBJ_FLAG_GESTURE_BUBBLE);
        actionButtonLabels[i] = lv_obj_get_child(actionButtons[i], 0);
        lv_obj_add_flag(actionButtonLabels[i], LV_OBJ_FLAG_GESTURE_BUBBLE);
    }

    const int quickPanelH = max(292, uiScaleY(304));
    const int quickStatsH = max(64, uiScaleY(70));
    const int quickInfoY = 92;
    const int quickInfoH = 92;

    quickPanel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(quickPanel, contentW, quickPanelH);
    lv_obj_align(quickPanel, LV_ALIGN_TOP_MID, 0, uiScaleY(72));
    lv_obj_set_style_radius(quickPanel, 24, 0);
    lv_obj_set_style_bg_color(quickPanel, lv_color_hex(0x102238), 0);
    lv_obj_set_style_bg_opa(quickPanel, LV_OPA_90, 0);
    lv_obj_set_style_border_width(quickPanel, 1, 0);
    lv_obj_set_style_border_color(quickPanel, lv_color_hex(0x8bb8e4), 0);
    lv_obj_set_style_pad_all(quickPanel, 0, 0);
    lv_obj_clear_flag(quickPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(quickPanel, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(quickPanel, onQuickPanel, LV_EVENT_RELEASED, nullptr);

    quickPanelStatsCard = lv_obj_create(quickPanel);
    lv_obj_set_size(quickPanelStatsCard, contentW - 32, quickStatsH);
    lv_obj_align(quickPanelStatsCard, LV_ALIGN_TOP_MID, 0, 14);
    lv_obj_set_style_radius(quickPanelStatsCard, 18, 0);
    lv_obj_set_style_bg_color(quickPanelStatsCard, lv_color_hex(0x162536), 0);
    lv_obj_set_style_bg_opa(quickPanelStatsCard, LV_OPA_40, 0);
    lv_obj_set_style_border_width(quickPanelStatsCard, 1, 0);
    lv_obj_set_style_border_color(quickPanelStatsCard, lv_color_hex(0x7ea6c8), 0);
    lv_obj_clear_flag(quickPanelStatsCard, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(quickPanelStatsCard, LV_OBJ_FLAG_GESTURE_BUBBLE);

    const int quickStatW = contentW - 32;
    const int quickStatColW = (quickStatW - 28) / 2;
    for (int i = 0; i < 4; ++i) {
        int col = i % 2;
        int row = i / 2;
        lv_obj_t *label = lv_label_create(quickPanelStatsCard);
        lv_label_set_text(label, statNames[i]);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 14 + col * quickStatColW, 8 + row * 26);
        lv_obj_set_style_text_font(label, zhFont, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xf7fbff), 0);

        quickPanelStatValues[i] = lv_label_create(quickPanelStatsCard);
        lv_obj_align(quickPanelStatValues[i], LV_ALIGN_TOP_LEFT, 64 + col * quickStatColW, 8 + row * 26);
        lv_obj_set_style_text_font(quickPanelStatValues[i], statNumberFont, 0);
        lv_obj_set_style_text_color(quickPanelStatValues[i], lv_color_hex(statColors[i]), 0);
    }

    lv_obj_t *quickInfoCard = lv_obj_create(quickPanel);
    lv_obj_set_size(quickInfoCard, contentW - 32, quickInfoH);
    lv_obj_align(quickInfoCard, LV_ALIGN_TOP_MID, 0, quickInfoY);
    lv_obj_set_style_radius(quickInfoCard, 16, 0);
    lv_obj_set_style_bg_color(quickInfoCard, lv_color_hex(0x0b1726), 0);
    lv_obj_set_style_bg_opa(quickInfoCard, LV_OPA_50, 0);
    lv_obj_set_style_border_width(quickInfoCard, 1, 0);
    lv_obj_set_style_border_color(quickInfoCard, lv_color_hex(0x456b8c), 0);
    lv_obj_set_style_pad_all(quickInfoCard, 10, 0);
    lv_obj_clear_flag(quickInfoCard, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(quickInfoCard, LV_OBJ_FLAG_GESTURE_BUBBLE);

    quickPanelLabel = lv_label_create(quickInfoCard);
    lv_obj_set_size(quickPanelLabel, contentW - 52, quickInfoH - 20);
    lv_label_set_long_mode(quickPanelLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(quickPanelLabel, zhFont, 0);
    lv_obj_set_style_text_color(quickPanelLabel, lv_color_hex(0xf7fbff), 0);
    lv_obj_set_style_text_align(quickPanelLabel, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(quickPanelLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    settingsPanel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(settingsPanel, contentW, max(292, uiScaleY(304)));
    lv_obj_align(settingsPanel, LV_ALIGN_TOP_MID, 0, uiScaleY(72));
    lv_obj_set_style_radius(settingsPanel, 24, 0);
    lv_obj_set_style_bg_color(settingsPanel, lv_color_hex(0x102238), 0);
    lv_obj_set_style_bg_opa(settingsPanel, LV_OPA_90, 0);
    lv_obj_set_style_border_width(settingsPanel, 1, 0);
    lv_obj_set_style_border_color(settingsPanel, lv_color_hex(0x8bb8e4), 0);
    lv_obj_set_style_pad_top(settingsPanel, 14, 0);
    lv_obj_set_style_pad_bottom(settingsPanel, 14, 0);
    lv_obj_set_style_pad_left(settingsPanel, 16, 0);
    lv_obj_set_style_pad_right(settingsPanel, 16, 0);
    lv_obj_set_style_pad_row(settingsPanel, 10, 0);
    lv_obj_set_flex_flow(settingsPanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(settingsPanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(settingsPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(settingsPanel, LV_OBJ_FLAG_GESTURE_BUBBLE);

    settingsStatusLabel = lv_label_create(settingsPanel);
    lv_obj_set_width(settingsStatusLabel, lv_pct(100));
    lv_obj_set_height(settingsStatusLabel, 44);
    lv_label_set_long_mode(settingsStatusLabel, LV_LABEL_LONG_DOT);
    lv_label_set_text(settingsStatusLabel, "设置：请选择操作");
    lv_obj_set_style_text_font(settingsStatusLabel, zhFont, 0);
    lv_obj_set_style_text_color(settingsStatusLabel, lv_color_hex(0xf7fbff), 0);
    lv_obj_set_style_text_align(settingsStatusLabel, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *quickVolumeRow = lv_obj_create(settingsPanel);
    lv_obj_set_size(quickVolumeRow, contentW - 32, 48);
    lv_obj_set_style_bg_opa(quickVolumeRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(quickVolumeRow, 0, 0);
    lv_obj_set_style_pad_all(quickVolumeRow, 0, 0);
    lv_obj_set_style_pad_column(quickVolumeRow, 12, 0);
    lv_obj_set_flex_flow(quickVolumeRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(quickVolumeRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(quickVolumeRow, LV_OBJ_FLAG_SCROLLABLE);

    quickVolumeDownBtn = lv_btn_create(quickVolumeRow);
    lv_obj_set_size(quickVolumeDownBtn, (contentW - 58) / 2, 46);
    lv_obj_set_style_radius(quickVolumeDownBtn, 12, 0);
    lv_obj_set_style_bg_color(quickVolumeDownBtn, lv_color_hex(0x2b4766), 0);
    lv_obj_set_style_border_width(quickVolumeDownBtn, 1, 0);
    lv_obj_set_style_border_color(quickVolumeDownBtn, lv_color_hex(0x8bb8e4), 0);
    lv_obj_add_event_cb(quickVolumeDownBtn, onQuickPanelAction, LV_EVENT_RELEASED, reinterpret_cast<void *>(static_cast<uintptr_t>(0)));
    lv_obj_t *quickVolumeDownLabel = lv_label_create(quickVolumeDownBtn);
    lv_label_set_text(quickVolumeDownLabel, "音量 -");
    lv_obj_set_style_text_font(quickVolumeDownLabel, zhFont, 0);
    lv_obj_center(quickVolumeDownLabel);

    quickVolumeUpBtn = lv_btn_create(quickVolumeRow);
    lv_obj_set_size(quickVolumeUpBtn, (contentW - 58) / 2, 46);
    lv_obj_set_style_radius(quickVolumeUpBtn, 12, 0);
    lv_obj_set_style_bg_color(quickVolumeUpBtn, lv_color_hex(0x2b4766), 0);
    lv_obj_set_style_border_width(quickVolumeUpBtn, 1, 0);
    lv_obj_set_style_border_color(quickVolumeUpBtn, lv_color_hex(0x8bb8e4), 0);
    lv_obj_add_event_cb(quickVolumeUpBtn, onQuickPanelAction, LV_EVENT_RELEASED, reinterpret_cast<void *>(static_cast<uintptr_t>(1)));
    lv_obj_t *quickVolumeUpLabel = lv_label_create(quickVolumeUpBtn);
    lv_label_set_text(quickVolumeUpLabel, "音量 +");
    lv_obj_set_style_text_font(quickVolumeUpLabel, zhFont, 0);
    lv_obj_center(quickVolumeUpLabel);

    lv_obj_t *quickButtonRow = lv_obj_create(settingsPanel);
    lv_obj_set_size(quickButtonRow, contentW - 32, 166);
    lv_obj_set_style_bg_opa(quickButtonRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(quickButtonRow, 0, 0);
    lv_obj_set_style_pad_all(quickButtonRow, 0, 0);
    lv_obj_set_style_pad_row(quickButtonRow, 10, 0);
    lv_obj_set_flex_flow(quickButtonRow, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(quickButtonRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(quickButtonRow, LV_OBJ_FLAG_SCROLLABLE);

    quickProvisionBtn = lv_btn_create(quickButtonRow);
    lv_obj_set_size(quickProvisionBtn, contentW - 32, 48);
    lv_obj_set_style_radius(quickProvisionBtn, 12, 0);
    lv_obj_set_style_bg_color(quickProvisionBtn, lv_color_hex(0x496e96), 0);
    lv_obj_add_event_cb(quickProvisionBtn, onQuickPanelAction, LV_EVENT_RELEASED, reinterpret_cast<void *>(static_cast<uintptr_t>(2)));
    quickProvisionBtnLabel = lv_label_create(quickProvisionBtn);
    lv_label_set_text(quickProvisionBtnLabel, PROVISION_BLE_ENABLED ? "蓝牙配网" : "开启配网");
    lv_obj_set_style_text_font(quickProvisionBtnLabel, zhFont, 0);
    lv_obj_center(quickProvisionBtnLabel);

    quickRetryBtn = lv_btn_create(quickButtonRow);
    lv_obj_set_size(quickRetryBtn, contentW - 32, 48);
    lv_obj_set_style_radius(quickRetryBtn, 12, 0);
    lv_obj_set_style_bg_color(quickRetryBtn, lv_color_hex(0x496e96), 0);
    lv_obj_add_event_cb(quickRetryBtn, onQuickPanelAction, LV_EVENT_RELEASED, reinterpret_cast<void *>(static_cast<uintptr_t>(3)));
    lv_obj_t *quickRetryLabel = lv_label_create(quickRetryBtn);
    lv_label_set_text(quickRetryLabel, "重试");
    lv_obj_set_style_text_font(quickRetryLabel, zhFont, 0);
    lv_obj_center(quickRetryLabel);

    quickOtaBtn = lv_btn_create(quickButtonRow);
    lv_obj_set_size(quickOtaBtn, contentW - 32, 48);
    lv_obj_set_style_radius(quickOtaBtn, 12, 0);
    lv_obj_set_style_bg_color(quickOtaBtn, lv_color_hex(0x496e96), 0);
    lv_obj_add_event_cb(quickOtaBtn, onQuickPanelAction, LV_EVENT_RELEASED, reinterpret_cast<void *>(static_cast<uintptr_t>(4)));
    lv_obj_t *quickOtaLabel = lv_label_create(quickOtaBtn);
    lv_label_set_text(quickOtaLabel, "SD刷机");
    lv_obj_set_style_text_font(quickOtaLabel, zhFont, 0);
    lv_obj_center(quickOtaLabel);

    lv_obj_add_flag(quickPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(settingsPanel, LV_OBJ_FLAG_HIDDEN);

    ttsStatusPanel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(ttsStatusPanel, min(328, contentW), max(68, uiScaleY(72)));
    lv_obj_align(ttsStatusPanel, LV_ALIGN_CENTER, 0, -8);
    lv_obj_set_style_radius(ttsStatusPanel, 22, 0);
    lv_obj_set_style_bg_color(ttsStatusPanel, lv_color_hex(0x271f13), 0);
    lv_obj_set_style_bg_opa(ttsStatusPanel, LV_OPA_90, 0);
    lv_obj_set_style_border_width(ttsStatusPanel, 1, 0);
    lv_obj_set_style_border_color(ttsStatusPanel, lv_color_hex(0xffc35c), 0);
    lv_obj_clear_flag(ttsStatusPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ttsStatusPanel, LV_OBJ_FLAG_GESTURE_BUBBLE);
    ttsStatusLabel = lv_label_create(ttsStatusPanel);
    lv_obj_set_width(ttsStatusLabel, min(292, contentW - 36));
    lv_label_set_long_mode(ttsStatusLabel, LV_LABEL_LONG_WRAP);
    lv_obj_center(ttsStatusLabel);
    lv_obj_set_style_text_align(ttsStatusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(ttsStatusLabel, zhFont, 0);
    lv_obj_set_style_text_color(ttsStatusLabel, lv_color_hex(0xfff0c2), 0);
    lv_label_set_text(ttsStatusLabel, "");
    lv_obj_add_flag(ttsStatusPanel, LV_OBJ_FLAG_HIDDEN);

    souvenirPanel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(souvenirPanel, min(320, contentW), max(140, uiScaleY(150)));
    lv_obj_align(souvenirPanel, LV_ALIGN_CENTER, 0, uiScaleY(18));
    lv_obj_set_style_radius(souvenirPanel, 26, 0);
    lv_obj_set_style_bg_color(souvenirPanel, lv_color_hex(0x172a3e), 0);
    lv_obj_set_style_bg_opa(souvenirPanel, LV_OPA_90, 0);
    lv_obj_set_style_border_width(souvenirPanel, 1, 0);
    lv_obj_set_style_border_color(souvenirPanel, lv_color_hex(0xffd48a), 0);
    lv_obj_set_style_shadow_width(souvenirPanel, 22, 0);
    lv_obj_set_style_shadow_color(souvenirPanel, lv_color_hex(0x05080c), 0);
    lv_obj_clear_flag(souvenirPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(souvenirPanel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(souvenirPanel, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(souvenirPanel, onSouvenirPanel, LV_EVENT_RELEASED, nullptr);

    souvenirTitleLabel = lv_label_create(souvenirPanel);
    lv_label_set_text(souvenirTitleLabel, "带回了纪念品");
    lv_obj_align(souvenirTitleLabel, LV_ALIGN_TOP_LEFT, 18, 14);
    lv_obj_set_style_text_font(souvenirTitleLabel, zhFont, 0);
    lv_obj_set_style_text_color(souvenirTitleLabel, lv_color_hex(0xffe4ad), 0);

    souvenirIndexLabel = lv_label_create(souvenirPanel);
    lv_obj_align(souvenirIndexLabel, LV_ALIGN_TOP_RIGHT, -18, 14);
    lv_obj_set_style_text_font(souvenirIndexLabel, statNumberFont, 0);
    lv_obj_set_style_text_color(souvenirIndexLabel, lv_color_hex(0xffe4ad), 0);

    souvenirPhotoFrame = lv_obj_create(souvenirPanel);
    lv_obj_set_size(souvenirPhotoFrame, 78, 58);
    lv_obj_align(souvenirPhotoFrame, LV_ALIGN_TOP_LEFT, 18, 48);
    lv_obj_set_style_radius(souvenirPhotoFrame, 8, 0);
    lv_obj_set_style_bg_color(souvenirPhotoFrame, lv_color_hex(0x8796a8), 0);
    lv_obj_set_style_bg_opa(souvenirPhotoFrame, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(souvenirPhotoFrame, 3, 0);
    lv_obj_set_style_border_color(souvenirPhotoFrame, lv_color_hex(0xfff0cf), 0);
    lv_obj_clear_flag(souvenirPhotoFrame, LV_OBJ_FLAG_SCROLLABLE);

    souvenirPhotoImage = lv_img_create(souvenirPhotoFrame);
    lv_obj_center(souvenirPhotoImage);
    lv_obj_add_flag(souvenirPhotoImage, LV_OBJ_FLAG_HIDDEN);

    souvenirPhotoLabel = lv_label_create(souvenirPhotoFrame);
    lv_label_set_text(souvenirPhotoLabel, "照片");
    lv_obj_center(souvenirPhotoLabel);
    lv_obj_set_style_text_font(souvenirPhotoLabel, zhFont, 0);
    lv_obj_set_style_text_color(souvenirPhotoLabel, lv_color_hex(0xffffff), 0);
    lv_obj_add_flag(souvenirPhotoFrame, LV_OBJ_FLAG_HIDDEN);

    souvenirItemLabel = lv_label_create(souvenirPanel);
    lv_obj_set_width(souvenirItemLabel, 284);
    lv_label_set_long_mode(souvenirItemLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(souvenirItemLabel, LV_ALIGN_TOP_LEFT, 18, 44);
    lv_obj_set_style_text_font(souvenirItemLabel, zhFont, 0);
    lv_obj_set_style_text_color(souvenirItemLabel, lv_color_hex(0xffffff), 0);

    souvenirNoteLabel = lv_label_create(souvenirPanel);
    lv_obj_set_width(souvenirNoteLabel, 284);
    lv_label_set_long_mode(souvenirNoteLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(souvenirNoteLabel, LV_ALIGN_TOP_LEFT, 18, 76);
    lv_obj_set_style_text_font(souvenirNoteLabel, zhFont, 0);
    lv_obj_set_style_text_color(souvenirNoteLabel, lv_color_hex(0xdce8f7), 0);

    souvenirHintLabel = lv_label_create(souvenirPanel);
    lv_obj_set_width(souvenirHintLabel, 284);
    lv_label_set_long_mode(souvenirHintLabel, LV_LABEL_LONG_DOT);
    lv_obj_align(souvenirHintLabel, LV_ALIGN_BOTTOM_LEFT, 18, -12);
    lv_obj_set_style_text_font(souvenirHintLabel, zhFont, 0);
    lv_obj_set_style_text_color(souvenirHintLabel, lv_color_hex(0xaec1d6), 0);
    lv_obj_add_flag(souvenirPanel, LV_OBJ_FLAG_HIDDEN);

    refreshUi();
}

void setup() {
    Serial.begin(115200);
    Config::load();
    deviceId = String("waveshare-touch-") + String(static_cast<uint32_t>(ESP.getEfuseMac() & 0xFFFFFF), HEX);

    Wire.begin(IIC_SDA, IIC_SCL);
    bool expanderOk = expander.begin(0x20);
    if (!expanderOk) {
        while (true) delay(1000);
    }
    expander.pinMode(0, OUTPUT);
    expander.pinMode(1, OUTPUT);
    expander.pinMode(2, OUTPUT);
    expander.pinMode(PWR_BUTTON_EXIO, INPUT);
    expander.pinMode(6, OUTPUT);
    expander.pinMode(7, OUTPUT);
    expander.digitalWrite(0, LOW);
    expander.digitalWrite(1, LOW);
    expander.digitalWrite(2, LOW);
    expander.digitalWrite(6, LOW);
    expander.digitalWrite(7, HIGH);
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    delay(20);
    expander.digitalWrite(0, HIGH);
    expander.digitalWrite(1, HIGH);
    expander.digitalWrite(2, HIGH);
    expander.digitalWrite(6, HIGH);

    while (!FT3168->begin()) {
        delay(250);
    }
    FT3168->IIC_Write_Device_State(
        FT3168->Arduino_IIC_Touch::Device::TOUCH_POWER_MODE,
        FT3168->Arduino_IIC_Touch::Device_Mode::TOUCH_POWER_MONITOR);

    gfx->begin();
    gfx->setBrightness(220);

    M5.Speaker.begin();
    setVolumeLevel(Config::getSpeakerVolume(), Config::getSpeakerVolume() != safeVolume(Config::getSpeakerVolume()));
    sdReady = PetStorage::begin();
    Serial.printf("[UI] SD panel status: %s, size=%lluMB\n", sdReady ? "ready" : "unavailable", PetStorage::cardSizeMB());
    serialSync.begin();
    SdOta::begin();
    if (!WAVESHARE_TTS_DISABLED) {
        localTts.begin();
    }
    weatherClient.begin(Config::getCity().length() > 0 ? Config::getCity() : String("Shenzhen"));
    weatherClientCity = Config::getCity().length() > 0 ? Config::getCity() : String("Shenzhen");
    weatherClientReady = true;
    if (!WAVESHARE_TTS_DISABLED) {
        ttsPlayback.begin(Config::getSttHost(), Config::getSttPort(), nullptr, 0);
        ttsPlayback.attachLocalTTS(&localTts);
    }

    companion.begin(companionCanvas);
    applyCachedWeatherFromConfig();
    wasOuting = companion.isOuting();
    lastSouvenirCount = companion.getSouvenirCount();

    lv_init();
    const size_t lvglDrawPixels = LCD_WIDTH * LVGL_DRAW_BUFFER_LINES;
    buf1 = static_cast<lv_color_t *>(heap_caps_malloc(lvglDrawPixels * sizeof(lv_color_t), MALLOC_CAP_DMA));
    buf2 = static_cast<lv_color_t *>(heap_caps_malloc(lvglDrawPixels * sizeof(lv_color_t), MALLOC_CAP_DMA));
    Serial.printf("[UI] LVGL draw buffers: lines=%d bytes_each=%u internal=%u largest8=%u\n",
                  LVGL_DRAW_BUFFER_LINES,
                  static_cast<unsigned>(lvglDrawPixels * sizeof(lv_color_t)),
                  heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    lv_disp_draw_buf_init(&drawBuf, buf1, buf2, lvglDrawPixels);

    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res = LCD_WIDTH;
    dispDrv.ver_res = LCD_HEIGHT;
    dispDrv.flush_cb = myDispFlush;
    dispDrv.draw_buf = &drawBuf;
    lv_disp_drv_register(&dispDrv);

    static lv_indev_drv_t indevDrv;
    lv_indev_drv_init(&indevDrv);
    indevDrv.type = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = myTouchRead;
    lv_indev_drv_register(&indevDrv);

    const esp_timer_create_args_t tickArgs = {
        .callback = &lvTick,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick"};
    esp_timer_handle_t tickTimer = nullptr;
    esp_timer_create(&tickArgs, &tickTimer);
    esp_timer_start_periodic(tickTimer, 2000);

    buildUi();
    lv_obj_add_event_cb(lv_scr_act(), handleRootGesture, LV_EVENT_GESTURE, nullptr);
    updateHomeBackgroundFromSd(true);
    refreshUi();
}

void loop() {
    companion.tick();
    serialSync.tick();
    SdOta::tick();
    serviceProvisioningSession();
    if (!provisionIsActive() && provisionCtx.state != ProvisionState::BleConnected) {
        serviceWeatherNetwork();
    }
    expireTownSyncIfNeeded();
    if (syncNetworkServicesStarted && WiFi.status() == WL_CONNECTED) {
        if (!touchAiInFlight) {
            cmdServer.tick();
            if (!townSyncActive) {
                int wType = companion.hasValidWeather() ? static_cast<int>(companion.getWeatherType()) : -1;
                float temp = companion.hasValidWeather() ? companion.getTemperature() : -999;
                stateBroadcastTick(static_cast<int>(companion.getState()),
                                   companion.getFrameIndex(), "TOUCH",
                                   companion.getNormX(), companion.getNormY(),
                                   companion.isFacingLeft() ? 1 : 0,
                                   wType, temp,
                                   3, companion.getHumidityPercent(),
                                   deviceId.c_str(), DEVICE_TYPE,
                                   DEVICE_CAP_TOUCH, DEVICE_CAP_KEYBOARD,
                                   DEVICE_CAP_MIC, DEVICE_CAP_SPEAKER);
            }
        }
    }
    serviceLocalTts();
    updateOutingReturn();
    pollHardwareButtons();
    serviceTouchCompanionAi();
    if (millis() - lastUiUpdateMs >= 50) {
        refreshUi();
        lastUiUpdateMs = millis();
    }
    lv_timer_handler();
    processPendingSwipe();
    delay(5);
}

#endif

