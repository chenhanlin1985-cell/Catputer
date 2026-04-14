#include "companion.h"
#include "sprites.h"
#include "sprites_purple.h"
#include "sprites_q.h"
#include "pet_dialogue.h"
#include <SD.h>
#include <time.h>

#if defined(CATPUTER_TOUCH_UI)
#define M5DEVICE M5
#else
#define M5DEVICE M5Cardputer
#endif

// Character draw dimensions
constexpr int CHAR_SCALE = 3;  // 16×3 = 48px on screen
constexpr int GROUND_Y = SCREEN_H - 28;

// Movement
constexpr int MOVE_STEP = 2;  // 2px per step (~120px/s at 60fps)
constexpr int MOVE_MIN_X = 0;
constexpr int MOVE_MIN_Y = 16;  // below clock area

// Day/night colors
constexpr uint16_t SKY_DAY    = rgb565(60, 120, 200);   // Blue sky
constexpr uint16_t SKY_SUNSET = rgb565(180, 80, 60);    // Orange sunset
constexpr uint16_t SKY_NIGHT  = rgb565(10, 10, 30);     // Dark night
constexpr uint16_t GROUND_DAY = rgb565(80, 140, 60);    // Green grass
constexpr uint16_t GROUND_DAY_TOP = rgb565(100, 170, 70);
constexpr uint16_t SUN_COLOR  = rgb565(255, 220, 60);
constexpr uint16_t MOON_COLOR = rgb565(220, 220, 200);
constexpr uint16_t CLOUD_COLOR = rgb565(220, 230, 240);

static uint8_t clampStat(int value) {
    if (value < 0) return 0;
    if (value > 100) return 100;
    return (uint8_t)value;
}

static bool isCooldownActive(unsigned long lastTime, unsigned long cooldownMs) {
    return lastTime != 0 && millis() - lastTime < cooldownMs;
}

static bool isPhotoSouvenir(const char* item) {
    return item && strncmp(item, "photo:", 6) == 0;
}

static const char* getPhotoKey(const char* item) {
    return isPhotoSouvenir(item) ? item + 6 : "";
}

static const char* getPhotoLabel(const char* key) {
    if (strcmp(key, "roof_sun") == 0) return u8"屋顶晒太阳";
    if (strcmp(key, "window_rain") == 0) return u8"窗边看雨";
    if (strcmp(key, "corner_store") == 0) return u8"街角小店";
    if (strcmp(key, "seaside_trip") == 0) return u8"海边散步";
    if (strcmp(key, "night_walk") == 0) return u8"夜路巡游";
    if (strcmp(key, "quiet_alley") == 0) return u8"安静小巷";
    return u8"外出照片";
}

struct SpriteSet {
    const uint16_t* const* idle;
    int idleCount;
    const uint16_t* const* happy;
    int happyCount;
    const uint16_t* const* sleep;
    int sleepCount;
    const uint16_t* const* talk;
    int talkCount;
    int spriteW;
    int spriteH;
    int scale;
};

static const SpriteSet ORANGE_SET = {
    idle_frames, IDLE_FRAME_COUNT,
    happy_frames, HAPPY_FRAME_COUNT,
    sleep_frames, SLEEP_FRAME_COUNT,
    talk_frames, TALK_FRAME_COUNT,
    CHAR_W, CHAR_H, CHAR_SCALE
};

static const SpriteSet PURPLE_SET = {
    purple_idle_frames, PURPLE_IDLE_FRAME_COUNT,
    purple_happy_frames, PURPLE_HAPPY_FRAME_COUNT,
    purple_sleep_frames, PURPLE_SLEEP_FRAME_COUNT,
    purple_talk_frames, PURPLE_TALK_FRAME_COUNT,
    CHAR_W, CHAR_H, CHAR_SCALE
};

static const SpriteSet Q_SET = {
    q_idle_frames, Q_IDLE_FRAME_COUNT,
    q_happy_frames, Q_HAPPY_FRAME_COUNT,
    q_sleep_frames, Q_SLEEP_FRAME_COUNT,
    q_talk_frames, Q_TALK_FRAME_COUNT,
    Q_CHAR_W, Q_CHAR_H, 2
};

static const SpriteSet& spriteSetForKind(const String& kind) {
    if (kind == "q") return Q_SET;
    if (kind == "purple") return PURPLE_SET;
    return ORANGE_SET;
}

static int charDrawWidthForKind(const String& kind) {
    const SpriteSet& s = spriteSetForKind(kind);
    return s.spriteW * s.scale;
}

static int charDrawHeightForKind(const String& kind) {
    const SpriteSet& s = spriteSetForKind(kind);
    return s.spriteH * s.scale;
}

static int moveMaxXForKind(const String& kind) {
    return SCREEN_W - charDrawWidthForKind(kind);
}

static int moveMaxYForKind(const String& kind) {
    return GROUND_Y - charDrawHeightForKind(kind) - 2;
}

static bool lineHasAllTokens(const char* line, const char* tokenA, const char* tokenB = nullptr) {
    if (!line || !tokenA) return false;
    if (strstr(line, tokenA) == nullptr) return false;
    if (tokenB && strstr(line, tokenB) == nullptr) return false;
    return true;
}

static const char* promptValue(const String& key, const char* fallback) {
    static char buf[128];
    String value = PetDialogue::value(key);
    if (value.length() == 0) return fallback;
    strncpy(buf, value.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    return buf;
}

static void drawSplitNote(M5Canvas& canvas, const char* note, int x, int y, uint16_t color) {
    const char* text = (note && note[0]) ? note : u8"带回一点小小纪念";
    const char* split = strchr(text, '|');
    canvas.setTextColor(color);
    if (!split) {
        canvas.drawString(text, x, y);
        return;
    }

    char line1[48];
    size_t line1Len = split - text;
    if (line1Len >= sizeof(line1)) line1Len = sizeof(line1) - 1;
    memcpy(line1, text, line1Len);
    line1[line1Len] = '\0';
    canvas.drawString(line1, x, y);
    canvas.drawString(split + 1, x, y + 14);
}

void Companion::begin(M5Canvas& canvas) {
    // Center position on first call; preserve across mode switches
    if (charX == 0 && charY == 0) {
        const int drawW = charDrawWidthForKind(petKind);
        charX = (SCREEN_W - drawW) / 2;
        charY = moveMaxYForKind(petKind);
    }
    loadPetProgress();
    loadSouvenirs();
    loadPromptMemory();
    PetDialogue::begin();
    initStars();
    setState(CompanionState::IDLE);
    spontaneousTimer.setInterval(8000 + random(7000)); // 8-15s
}

void Companion::loadPetProgress() {
    if (petProgressLoaded) return;
    petProgressLoaded = true;
    fullness = clampStat(Config::getPetFullness());
    mood = clampStat(Config::getPetMood());
    energy = clampStat(Config::getPetEnergy());
    cleanliness = clampStat(Config::getPetCleanliness());
    bond = clampStat(Config::getPetBond());
    petId = Config::getPetId();
    petName = Config::getPetName();
    petKind = Config::getPetKind();
    petPersonality = normalizePersonality(Config::getPetPersonality());
    if (petId.length() == 0) petId = "device-cat";
    if (petName.length() == 0) petName = "小橘";
    if (petKind.length() == 0) petKind = "orange";
    if (petPersonality.length() == 0) petPersonality = "lively";
    petProgressDirty = false;
}

void Companion::loadSouvenirs() {
    if (souvenirsLoaded) return;
    souvenirsLoaded = true;
    if (PetStorage::loadSouvenirs(souvenirItems, souvenirNotes, souvenirCount, MAX_SOUVENIR_SLOTS)) {
        return;
    }
    souvenirCount = Config::getSouvenirCount();
    if (souvenirCount > 3) souvenirCount = 3;
    bool sanitized = false;
    for (uint8_t i = 0; i < 3; i++) {
        const String& value = Config::getSouvenirSlot(i);
        strncpy(souvenirItems[i], value.c_str(), sizeof(souvenirItems[i]) - 1);
        souvenirItems[i][sizeof(souvenirItems[i]) - 1] = '\0';
        const String& note = Config::getSouvenirNoteSlot(i);
        strncpy(souvenirNotes[i], note.c_str(), sizeof(souvenirNotes[i]) - 1);
        souvenirNotes[i][sizeof(souvenirNotes[i]) - 1] = '\0';
        if (strchr(souvenirItems[i], '?') != nullptr) {
            strncpy(souvenirItems[i], "old souvenir", sizeof(souvenirItems[i]) - 1);
            souvenirItems[i][sizeof(souvenirItems[i]) - 1] = '\0';
            sanitized = true;
        }
        if (strchr(souvenirNotes[i], '?') != nullptr) {
            strncpy(souvenirNotes[i], "old note", sizeof(souvenirNotes[i]) - 1);
            souvenirNotes[i][sizeof(souvenirNotes[i]) - 1] = '\0';
            sanitized = true;
        }
    }
    if (sanitized) {
        for (uint8_t i = 0; i < 3; i++) {
            Config::setSouvenirSlot(i, souvenirItems[i]);
            Config::setSouvenirNoteSlot(i, souvenirNotes[i]);
        }
        Config::setSouvenirCount(souvenirCount);
        Config::save();
        PetStorage::saveSouvenirs(souvenirItems, souvenirNotes, souvenirCount, MAX_SOUVENIR_SLOTS);
    }
}

void Companion::loadPromptMemory() {
    if (promptMemoryLoaded) return;
    promptMemoryLoaded = true;
    for (uint8_t i = 0; i < PetStorage::PROMPT_SLOT_COUNT; i++) {
        promptAskedDayStamp[i] = -1;
        promptReplies[i][0] = '\0';
        promptFollowupAt[i] = 0;
        promptFollowupDayStamp[i] = -1;
    }
    PetStorage::loadPromptMemory(petId.c_str(), promptAskedDayStamp, promptReplies);
}

void Companion::savePromptMemory() {
    loadPromptMemory();
    PetStorage::savePromptMemory(petId.c_str(), promptAskedDayStamp, promptReplies);
}

void Companion::exportPromptMemory(int askedDayStamp[PetStorage::PROMPT_SLOT_COUNT],
                                   char replies[PetStorage::PROMPT_SLOT_COUNT][PetStorage::PROMPT_REPLY_LEN]) {
    loadPromptMemory();
    for (uint8_t i = 0; i < PetStorage::PROMPT_SLOT_COUNT; i++) {
        askedDayStamp[i] = promptAskedDayStamp[i];
        strncpy(replies[i], promptReplies[i], PetStorage::PROMPT_REPLY_LEN - 1);
        replies[i][PetStorage::PROMPT_REPLY_LEN - 1] = '\0';
    }
}

void Companion::importPromptMemory(const int askedDayStamp[PetStorage::PROMPT_SLOT_COUNT],
                                   const char replies[PetStorage::PROMPT_SLOT_COUNT][PetStorage::PROMPT_REPLY_LEN]) {
    loadPromptMemory();
    for (uint8_t i = 0; i < PetStorage::PROMPT_SLOT_COUNT; i++) {
        promptAskedDayStamp[i] = askedDayStamp[i];
        strncpy(promptReplies[i], replies[i], PetStorage::PROMPT_REPLY_LEN - 1);
        promptReplies[i][PetStorage::PROMPT_REPLY_LEN - 1] = '\0';
    }
    savePromptMemory();
}

void Companion::exportPromptFollowups(char lines[PetStorage::PROMPT_SLOT_COUNT][PetStorage::PROMPT_REPLY_LEN],
                                      uint8_t& count) const {
    count = 0;
    const int dayStamp = currentDayStamp();
    if (dayStamp < 0) return;

    for (uint8_t i = 0; i < PetStorage::PROMPT_SLOT_COUNT; i++) {
        lines[i][0] = '\0';
        if (promptFollowupAt[i] == 0) continue;
        if (promptFollowupDayStamp[i] != dayStamp) continue;

        const char* text = "";
        switch (static_cast<PromptSlot>(i)) {
            case PromptSlot::BREAKFAST:
                text = u8"之后还会轻轻提醒你吃早餐";
                break;
            case PromptSlot::LUNCH:
                text = u8"之后还会轻轻提醒你记得午饭";
                break;
            case PromptSlot::DINNER:
                text = u8"之后还会轻轻提醒你别忘了晚饭";
                break;
            case PromptSlot::LATE_NIGHT:
                text = u8"晚一点会再提醒你让眼睛休息";
                break;
            case PromptSlot::RANDOM_MOOD:
                text = u8"过一会儿还会再轻轻关心你";
                break;
            default:
                break;
        }
        if (!text[0]) continue;
        strncpy(lines[count], text, PetStorage::PROMPT_REPLY_LEN - 1);
        lines[count][PetStorage::PROMPT_REPLY_LEN - 1] = '\0';
        count++;
    }
}

void Companion::pushSouvenir(const char* item, const char* note) {
    loadSouvenirs();
    int upper = (souvenirCount < MAX_SOUVENIR_SLOTS) ? souvenirCount : (MAX_SOUVENIR_SLOTS - 1);
    for (int i = upper; i > 0; i--) {
        strncpy(souvenirItems[i], souvenirItems[i - 1], sizeof(souvenirItems[i]) - 1);
        souvenirItems[i][sizeof(souvenirItems[i]) - 1] = '\0';
        strncpy(souvenirNotes[i], souvenirNotes[i - 1], sizeof(souvenirNotes[i]) - 1);
        souvenirNotes[i][sizeof(souvenirNotes[i]) - 1] = '\0';
    }
    strncpy(souvenirItems[0], item, sizeof(souvenirItems[0]) - 1);
    souvenirItems[0][sizeof(souvenirItems[0]) - 1] = '\0';
    strncpy(souvenirNotes[0], note, sizeof(souvenirNotes[0]) - 1);
    souvenirNotes[0][sizeof(souvenirNotes[0]) - 1] = '\0';
    if (souvenirCount < MAX_SOUVENIR_SLOTS) souvenirCount++;
    for (uint8_t i = 0; i < 3; i++) {
        Config::setSouvenirSlot(i, souvenirItems[i]);
        Config::setSouvenirNoteSlot(i, souvenirNotes[i]);
    }
    Config::setSouvenirCount(souvenirCount > 3 ? 3 : souvenirCount);
    Config::save();
    PetStorage::saveSouvenirs(souvenirItems, souvenirNotes, souvenirCount, MAX_SOUVENIR_SLOTS);
    PetStorage::appendEventLog("souvenir", item);
}

void Companion::markPetProgressDirty() {
    petProgressDirty = true;
    petSaveTimer.reset();
}

void Companion::setTemporaryStatus(const char* text, unsigned long durationMs) {
    strncpy(temporaryStatus, text, sizeof(temporaryStatus) - 1);
    temporaryStatus[sizeof(temporaryStatus) - 1] = '\0';
    temporaryStatusUntil = millis() + durationMs;
    ambientStatusRefreshPending = true;
#if !defined(CATPUTER_WAVESHARE_AMOLED_18)
    queueSpeechLine(text);
#endif
}

void Companion::queueSpeechLine(const char* text) {
    if (!text || !text[0]) return;

    // Don't speak raw choice strings or placeholders.
    if (strcmp(text, "1") == 0 || strcmp(text, "2") == 0 || strcmp(text, "3") == 0) return;

    unsigned long now = millis();
    if (strcmp(lastSpokenFeedback, text) == 0 && (now - lastSpeechQueuedAt) < 1200) return;

    pendingSpeechCount = 0;
    for (uint8_t i = 0; i < MAX_PENDING_SPEECH_SEGMENTS; ++i) {
        pendingSpeech[i][0] = '\0';
    }

    auto utf8CharLen = [](unsigned char c) -> uint8_t {
        if ((c & 0x80) == 0x00) return 1;
        if ((c & 0xE0) == 0xC0) return 2;
        if ((c & 0xF0) == 0xE0) return 3;
        if ((c & 0xF8) == 0xF0) return 4;
        return 1;
    };
    char* dst = pendingSpeech[0];
    size_t dstCap = sizeof(pendingSpeech[0]) - 1;
    size_t dstLen = 0;
    bool justAddedPause = false;
    const char* p = text;
    while (*p && dstLen < dstCap) {
        uint8_t clen = utf8CharLen((unsigned char)*p);
        if (clen == 1) {
            char c = *p;
            if (c == '|' || c == '\n' || c == '\r' || c == ',' || c == '.' ||
                c == '!' || c == '?' || c == ';' || c == ':') {
                static const char kPause[] = "\xE3\x80\x82";  // 。
                if (!justAddedPause && dstLen + 3 <= dstCap) {
                    memcpy(dst + dstLen, kPause, 3);
                    dstLen += 3;
                    justAddedPause = true;
                }
                p++;
                continue;
            }
            if ((c >= 0x20 && c <= 0x7E)) {
                p++;
                continue;
            }
        }

        if (dstLen + clen > dstCap) break;
        memcpy(dst + dstLen, p, clen);
        dstLen += clen;
        p += clen;
        justAddedPause = false;
    }

    dst[dstLen] = '\0';
    if (dstLen > 0) pendingSpeechCount = 1;

    strncpy(lastSpokenFeedback, text, sizeof(lastSpokenFeedback) - 1);
    lastSpokenFeedback[sizeof(lastSpokenFeedback) - 1] = '\0';
    lastSpeechQueuedAt = now;
}

bool Companion::takePendingSpeech(char* out, size_t outSize) {
    if (!out || outSize == 0 || pendingSpeechCount == 0 || pendingSpeech[0][0] == '\0') return false;
    strncpy(out, pendingSpeech[0], outSize - 1);
    out[outSize - 1] = '\0';
    for (uint8_t i = 1; i < pendingSpeechCount; ++i) {
        strncpy(pendingSpeech[i - 1], pendingSpeech[i], sizeof(pendingSpeech[i - 1]) - 1);
        pendingSpeech[i - 1][sizeof(pendingSpeech[i - 1]) - 1] = '\0';
    }
    if (pendingSpeechCount > 0) {
        pendingSpeech[pendingSpeechCount - 1][0] = '\0';
        pendingSpeechCount--;
    }
    return true;
}

void Companion::speak(const char* text) {
    queueSpeechLine(text);
}

void Companion::pushStoryBeat(const char* text) {
    if (!text || !text[0]) return;
    if (storyBeatCount > 0 && strcmp(storyBeats[0], text) == 0) return;

    int upper = storyBeatCount < MAX_STORY_BEATS ? storyBeatCount : (MAX_STORY_BEATS - 1);
    for (int i = upper; i > 0; --i) {
        strncpy(storyBeats[i], storyBeats[i - 1], sizeof(storyBeats[i]) - 1);
        storyBeats[i][sizeof(storyBeats[i]) - 1] = '\0';
    }
    strncpy(storyBeats[0], text, sizeof(storyBeats[0]) - 1);
    storyBeats[0][sizeof(storyBeats[0]) - 1] = '\0';
    if (storyBeatCount < MAX_STORY_BEATS) storyBeatCount++;
}

void Companion::maybeShareReturnSummary() {
    if (storyBeatCount == 0) return;
    if (activePromptSlot != PromptSlot::NONE || promptTextEntryActive) return;

    showNotification(u8"\u5c0f\u732b", u8"\u4f60\u56de\u6765\u5566", storyBeats[0]);
    if (storyBeatCount > 1) {
        setTemporaryStatus(storyBeats[1], 2600);
    } else {
        setTemporaryStatus(u8"\u521a\u624d\u7684\u4e8b\u60f3\u8ddf\u4f60\u8bf4", 2200);
    }

    storyBeatCount = 0;
    for (uint8_t i = 0; i < MAX_STORY_BEATS; i++) {
        storyBeats[i][0] = '\0';
    }
}

void Companion::noteUserAttention() {
    unsigned long now = millis();
    bool returning = lastAttentionAt != 0 && (now - lastAttentionAt) >= ATTENTION_RETURN_MS;
    lastAttentionAt = now;
    if (returning) maybeShareReturnSummary();
}

bool Companion::buildAttentionSessionText(char* out, size_t outSize) {
    if (!out || outSize == 0) return false;
    out[0] = '\0';

    unsigned long now = millis();
    bool returning = lastAttentionAt != 0 && (now - lastAttentionAt) >= ATTENTION_RETURN_MS;
    lastAttentionAt = now;
    if (!returning) return false;
    if (activePromptSlot != PromptSlot::NONE || promptTextEntryActive || townSyncActive || outingActive || focusModeActive) {
        return false;
    }

    String text = personalityAmbient("return");
    if (storyBeatCount > 0 && storyBeats[0][0]) {
        text += "\n";
        text += storyBeats[0];
    } else {
        const char* ambient = recentMemoryAmbient("idle");
        if (ambient && ambient[0]) {
            text += "\n";
            text += ambient;
        } else {
            text += "\n";
            text += u8"\u6211\u521a\u624d\u5728\u623f\u95f4\u91cc\u6162\u6162\u5f85\u7740";
        }
    }

    const char* follow = recentMemoryAmbient("talk");
    if (follow && follow[0] && text.indexOf(follow) < 0) {
        text += "\n";
        text += follow;
    }

    text.toCharArray(out, outSize);
    storyBeatCount = 0;
    for (uint8_t i = 0; i < MAX_STORY_BEATS; i++) {
        storyBeats[i][0] = '\0';
    }
    return true;
}

void Companion::savePetProgress(bool force) {
    if (!petProgressLoaded || !petProgressDirty) return;
    if (!force && !petSaveTimer.tick()) return;
    Config::setPetFullness(fullness);
    Config::setPetMood(mood);
    Config::setPetEnergy(energy);
    Config::setPetCleanliness(cleanliness);
    Config::setPetBond(bond);
    Config::setPetId(petId);
    Config::setPetName(petName);
    Config::setPetKind(petKind);
    Config::setPetPersonality(petPersonality);
    Config::save();
    char detail[96];
    snprintf(detail, sizeof(detail), "F=%u M=%u E=%u C=%u B=%u",
             fullness, mood, energy, cleanliness, bond);
    PetStorage::appendEventLog("pet_state", detail);
    petProgressDirty = false;
}

void Companion::showSouvenirs() {
    noteUserAttention();
    loadSouvenirs();
    maybePromptOnInteraction();
    if (townSyncActive) {
        setTemporaryStatus(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", 1400);
        return;
    }
    if (outingActive) {
        setTemporaryStatus(u8"\u732b\u54aa\u5916\u51fa\u4e2d", 1400);
        return;
    }
    if (souvenirCount == 0) {
        setTemporaryStatus(u8"\u8fd8\u6ca1\u6709\u7eaa\u5ff5", 1600);
        showNotification(u8"\u732b\u54aa", u8"\u7eaa\u5ff5\u76d2", u8"\u5148\u53bb\u6563\u6563\u6b65\u5427");
        return;
    }
    if (souvenirViewerUntil > millis()) {
        souvenirViewIndex = (souvenirViewIndex + 1) % souvenirCount;
    } else {
        souvenirViewIndex = 0;
    }
    souvenirViewerUntil = millis() + 4500;
    setTemporaryStatus(u8"\u6253\u5f00\u7eaa\u5ff5\u76d2", 1400);
}

const char* Companion::getSouvenirItem(uint8_t index) const {
    return index < souvenirCount ? souvenirItems[index] : "";
}

const char* Companion::getSouvenirNote(uint8_t index) const {
    return index < souvenirCount ? souvenirNotes[index] : "";
}

void Companion::setTownSyncActive(bool active) {
    if (townSyncActive == active) return;
    townSyncActive = active;
    if (active) {
        outingActive = false;
        toyGameActive = false;
        setTemporaryStatus(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", 2500);
    } else {
        setTemporaryStatus(u8"\u5c0f\u732b\u56de\u6765\u4e86", 2500);
        if (state == CompanionState::TALK) setState(CompanionState::IDLE);
    }
}

void Companion::applySyncSnapshot(uint8_t newFullness, uint8_t newMood, uint8_t newEnergy,
                                  uint8_t newCleanliness, uint8_t newBond, int newX, int newY,
                                  bool newFacingLeft, bool sleeping, const char* status,
                                  const char* newPetId, const char* newPetName, const char* newPetKind,
                                  const char* newPetPersonality) {
    loadPetProgress();
    fullness = clampStat(newFullness);
    mood = clampStat(newMood);
    energy = clampStat(newEnergy);
    cleanliness = clampStat(newCleanliness);
    bond = clampStat(newBond);
    if (newPetId && newPetId[0]) petId = newPetId;
    if (newPetName && newPetName[0]) petName = newPetName;
    if (newPetKind && newPetKind[0]) petKind = newPetKind;
    if (newPetPersonality && newPetPersonality[0]) petPersonality = normalizePersonality(newPetPersonality);
    charX = newX;
    charY = newY;
    const int moveMaxX = moveMaxXForKind(petKind);
    const int moveMaxY = moveMaxYForKind(petKind);
    if (charX < MOVE_MIN_X) charX = MOVE_MIN_X;
    if (charX > moveMaxX) charX = moveMaxX;
    if (charY < MOVE_MIN_Y) charY = MOVE_MIN_Y;
    if (charY > moveMaxY) charY = moveMaxY;
    facingLeft = newFacingLeft;
    promptMemoryLoaded = false;
    loadPromptMemory();
    activePromptSlot = PromptSlot::NONE;
    promptTextEntryActive = false;
    promptInput[0] = '\0';
    outingActive = false;
    toyGameActive = false;
    setState(sleeping ? CompanionState::SLEEP : CompanionState::IDLE);
    if (status && status[0]) {
        setTemporaryStatus(status, 5000);
    }
    markPetProgressDirty();
    savePetProgress(true);
}

String Companion::normalizePersonality(const String& value) const {
    if (value == "clingy" || value == "calm" || value == "lively" || value == "aloof") {
        return value;
    }
    return "lively";
}

const char* Companion::affectionLabel() const {
    if (bond >= 75) return u8"依赖";
    if (bond >= 50) return u8"亲近";
    if (bond >= 25) return u8"熟悉";
    return u8"陌生";
}

const char* Companion::personalityAmbient(const char* category) const {
    static String sdLine;
    sdLine = PetDialogue::pick(petPersonality, category);
    if (sdLine.length() > 0) {
        return sdLine.c_str();
    }

    const bool isClingy = petPersonality == "clingy";
    const bool isCalm = petPersonality == "calm";
    const bool isAloof = petPersonality == "aloof";

    if (strcmp(category, "happy") == 0) {
        if (isClingy) return u8"贴贴一下!";
        if (isCalm) return u8"这样就很好";
        if (isAloof) return u8"哼, 还不错";
        return u8"开心到摇尾巴!";
    }
    if (strcmp(category, "idle") == 0) {
        if (isClingy) return (bond >= 60) ? u8"我就待在你旁边" : u8"想离你近一点";
        if (isCalm) return u8"慢慢待着就很好";
        if (isAloof) return u8"我只是在这里看看";
        return u8"今天想动一动";
    }
    if (strcmp(category, "talk") == 0) {
        if (isClingy) return u8"我想先跟你说话";
        if (isCalm) return u8"嗯, 慢慢说吧";
        if (isAloof) return u8"你说的话, 我会听";
        return u8"来聊天呀";
    }
    if (strcmp(category, "look") == 0) {
        if (isClingy) return u8"我在找你的方向";
        if (isCalm) return u8"我在看看四周";
        if (isAloof) return u8"只是随便扫一眼";
        return u8"耳朵都竖起来了";
    }
    if (strcmp(category, "lonely") == 0) {
        if (isClingy) return u8"再陪我一下嘛";
        if (isCalm) return u8"想和你待一会儿";
        if (isAloof) return u8"我不是想你, 只是有点安静";
        return u8"来陪我玩嘛";
    }
    if (strcmp(category, "return") == 0) {
        if (isClingy) return u8"你回来啦, 我刚好想靠近一点";
        if (isCalm) return u8"你回来啦, 我刚才安静待着";
        if (isAloof) return u8"你回来啦, 我也只是刚好在这";
        return u8"你回来啦, 我正想告诉你一件小事";
    }
    return u8"喵";
}

int Companion::currentDayStamp() const {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return -1;
    return (timeinfo.tm_year + 1900) * 1000 + timeinfo.tm_yday;
}

const char* Companion::promptSlotMemoryKey(PromptSlot slot) const {
    switch (slot) {
        case PromptSlot::MORNING: return "morning";
        case PromptSlot::BREAKFAST: return "breakfast";
        case PromptSlot::LUNCH: return "lunch";
        case PromptSlot::DINNER: return "dinner";
        case PromptSlot::LATE_NIGHT: return "late";
        case PromptSlot::RANDOM_MOOD: return "mood";
        default: return "prompt";
    }
}

bool Companion::canTriggerPrompt(PromptSlot slot, int dayStamp) const {
    int idx = static_cast<int>(slot);
    if (idx < 0 || idx >= PetStorage::PROMPT_SLOT_COUNT) return false;
    return promptAskedDayStamp[idx] != dayStamp;
}

bool Companion::isPromptCardSlot(PromptSlot slot) const {
    switch (slot) {
        case PromptSlot::BREAKFAST:
        case PromptSlot::LUNCH:
        case PromptSlot::DINNER:
        case PromptSlot::LATE_NIGHT:
        case PromptSlot::RANDOM_MOOD:
            return true;
        default:
            return false;
    }
}

void Companion::markPromptAsked(PromptSlot slot, int dayStamp) {
    int idx = static_cast<int>(slot);
    if (idx < 0 || idx >= PetStorage::PROMPT_SLOT_COUNT) return;
    promptAskedDayStamp[idx] = dayStamp;
    savePromptMemory();
}

const char* Companion::promptSlotQuestion(PromptSlot slot) const {
    const char* recall = promptSlotRecallQuestion(slot);
    if (recall && recall[0]) return recall;

    switch (slot) {
        case PromptSlot::MORNING: return promptValue("prompt.question.morning", u8"早安呀|今天想先做什么?");
        case PromptSlot::BREAKFAST: return promptValue("prompt.question.breakfast", u8"九点啦|你吃早饭了吗?");
        case PromptSlot::LUNCH: return promptValue("prompt.question.lunch", u8"到中午了|要不要吃点东西?");
        case PromptSlot::DINNER: return promptValue("prompt.question.dinner", u8"傍晚了|晚饭想吃什么?");
        case PromptSlot::LATE_NIGHT: return promptValue("prompt.question.late_night", u8"已经很晚了|要不要早点睡?");
        case PromptSlot::RANDOM_MOOD: return promptValue("prompt.question.random_mood", u8"现在心情怎么样?|想和我说说吗?");
        default: return "";
    }
}

const char* Companion::classifyPromptReply(PromptSlot slot, const char* replyText) const {
    if (!replyText || !replyText[0]) return "";

    switch (slot) {
        case PromptSlot::MORNING:
            if (strstr(replyText, u8"困")) return "sleepy";
            if (strstr(replyText, u8"忙")) return "busy";
            return "greet";
        case PromptSlot::BREAKFAST:
        case PromptSlot::LUNCH:
        case PromptSlot::DINNER:
            if (strstr(replyText, u8"吃")) return "ate";
            if (strstr(replyText, u8"等")) return "later";
            if (strstr(replyText, u8"饿")) return "not_hungry";
            return "meal";
        case PromptSlot::LATE_NIGHT:
            if (strstr(replyText, u8"睡")) return "sleep_now";
            if (strstr(replyText, u8"再玩")) return "play_more";
            if (strstr(replyText, u8"不困")) return "awake";
            return "late";
        case PromptSlot::RANDOM_MOOD:
            if (strstr(replyText, u8"开心")) return "happy";
            if (strstr(replyText, u8"不开心")) return "sad";
            if (strstr(replyText, u8"不知道")) return "unsure";
            return "mood";
        default:
            return "";
    }
}

const char* Companion::promptSlotRecallQuestion(PromptSlot slot) const {
    static char buf[64];
    buf[0] = '\0';

    int idx = static_cast<int>(slot);
    if (idx < 0 || idx >= PetStorage::PROMPT_SLOT_COUNT) return "";
    const char* reply = promptReplies[idx];
    if (!reply[0]) return "";

    const char* tag = classifyPromptReply(slot, reply);

    switch (slot) {
        case PromptSlot::MORNING:
            if (strcmp(tag, "sleepy") == 0) return promptValue("prompt.recall.morning.sleepy", u8"早安呀|昨天你说还困, 今天好点了吗?");
            if (strcmp(tag, "busy") == 0) return promptValue("prompt.recall.morning.busy", u8"早安呀|今天也会忙忙的吗?");
            return promptValue("prompt.recall.morning.default", u8"早安呀|又想先听你说一声早呀");
        case PromptSlot::BREAKFAST:
            if (strcmp(tag, "ate") == 0) return promptValue("prompt.recall.breakfast.ate", u8"九点啦|上次你有好好吃早饭, 今天也有吗?");
            if (strcmp(tag, "later") == 0) return promptValue("prompt.recall.breakfast.later", u8"九点啦|上次你说等会吃, 今天别忘啦");
            if (strcmp(tag, "not_hungry") == 0) return promptValue("prompt.recall.breakfast.not_hungry", u8"九点啦|今天肚子有比较想吃点东西吗?");
            return promptValue("prompt.recall.breakfast.default", u8"九点啦|我又来问问你的早饭");
        case PromptSlot::LUNCH:
            if (strcmp(tag, "ate") == 0) return promptValue("prompt.recall.lunch.ate", u8"到中午了|上次你有去吃饭, 今天也要记得哦");
            if (strcmp(tag, "later") == 0) return promptValue("prompt.recall.lunch.later", u8"到中午了|上次你想稍后吃, 今天别拖太久呀");
            if (strcmp(tag, "not_hungry") == 0) return promptValue("prompt.recall.lunch.not_hungry", u8"到中午了|现在有没有比刚才更想吃一点?");
            return promptValue("prompt.recall.lunch.default", u8"到中午了|想再确认一下你有没有吃饭");
        case PromptSlot::DINNER:
            if (strcmp(tag, "ate") == 0) return promptValue("prompt.recall.dinner.ate", u8"傍晚了|你上次有好好吃晚饭, 今晚也继续吗?");
            if (strcmp(tag, "later") == 0) return promptValue("prompt.recall.dinner.later", u8"傍晚了|这次会不会又想等会再吃呀?");
            if (strcmp(tag, "not_hungry") == 0) return promptValue("prompt.recall.dinner.not_hungry", u8"傍晚了|现在还是不太饿吗?");
            return promptValue("prompt.recall.dinner.default", u8"傍晚了|想来问问今晚的肚子");
        case PromptSlot::LATE_NIGHT:
            if (strcmp(tag, "sleep_now") == 0) return promptValue("prompt.recall.late_night.sleep_now", u8"已经很晚了|昨晚你有早点睡, 今晚也保持吗?");
            if (strcmp(tag, "play_more") == 0) return promptValue("prompt.recall.late_night.play_more", u8"已经很晚了|昨晚你还想再玩会, 今晚也一样吗?");
            if (strcmp(tag, "awake") == 0) return promptValue("prompt.recall.late_night.awake", u8"已经很晚了|你昨天说还不困, 现在呢?");
            return promptValue("prompt.recall.late_night.default", u8"已经很晚了|今天想早点休息吗?");
        case PromptSlot::RANDOM_MOOD:
            if (strcmp(tag, "happy") == 0) return promptValue("prompt.recall.random_mood.happy", u8"现在心情怎么样?|上次你说挺开心, 今天也一样吗?");
            if (strcmp(tag, "sad") == 0) return promptValue("prompt.recall.random_mood.sad", u8"现在心情怎么样?|上次你有点不开心, 今天好一点了吗?");
            if (strcmp(tag, "unsure") == 0) return promptValue("prompt.recall.random_mood.unsure", u8"现在心情怎么样?|上次你说有点说不上来, 今天呢?");
            return promptValue("prompt.recall.random_mood.default", u8"现在心情怎么样?|想和我再说说吗?");
        default:
            return "";
    }
}

const char* Companion::recentMemoryAmbient(const char* category) const {
    static char buf[64];
    buf[0] = '\0';

    char recentLines[6][96] = {{0}};
    uint8_t recentCount = 0;
    bool hasRecentLog = PetStorage::loadRecentPetMemoryEvents(petId.c_str(), recentLines, recentCount, 6);

    if (hasRecentLog) {
        for (int i = recentCount - 1; i >= 0; --i) {
            const char* line = recentLines[i];
            if (strcmp(category, "happy") == 0) {
                if (lineHasAllTokens(line, "action | play_chase")) return u8"刚才追着玩具跑, 现在还想蹦一蹦";
                if (lineHasAllTokens(line, "action | play")) return u8"你刚才陪我玩过, 我还开心着";
                if (lineHasAllTokens(line, "reply_quick", "mood\t开心") || lineHasAllTokens(line, "reply_custom", "开心")) {
                    return u8"想到你刚才说开心, 我也想摇尾巴";
                }
            }
            if (strcmp(category, "idle") == 0) {
                if (lineHasAllTokens(line, "action | feed")) return u8"刚才吃过东西, 肚子暖暖的";
                if (lineHasAllTokens(line, "action | clean")) return u8"身上还是香香的, 想慢慢待着";
                if (lineHasAllTokens(line, "action | nap")) return u8"刚才那一小觉, 睡得挺舒服";
            }
            if (strcmp(category, "talk") == 0) {
                if (lineHasAllTokens(line, "reply_quick", "mood\t不开心") || lineHasAllTokens(line, "reply_custom", "不开心")) {
                    return u8"要是你还闷闷的, 也可以继续和我说";
                }
                if (lineHasAllTokens(line, "reply_quick", "mood\t不知道") || lineHasAllTokens(line, "reply_custom", "不知道")) {
                    return u8"说不上来也没关系, 我会听着";
                }
                if (lineHasAllTokens(line, "prompt | followup:")) {
                    return u8"我刚刚还在想着要轻轻提醒你";
                }
            }
            if (strcmp(category, "look") == 0) {
                if (lineHasAllTokens(line, "action | outing_return")) return u8"刚带着一点小见闻回来, 还想再看看";
                if (lineHasAllTokens(line, "reply_quick", "late\t再玩会") || lineHasAllTokens(line, "reply_custom", "再玩")) {
                    return u8"你说还想再玩会儿, 我就先陪你看看四周";
                }
            }
        }
    }

    if (strcmp(category, "happy") != 0) return "";

    const char* moodReply = promptReplies[static_cast<int>(PromptSlot::RANDOM_MOOD)];
    const char* lateReply = promptReplies[static_cast<int>(PromptSlot::LATE_NIGHT)];
    const char* mealReply = promptReplies[static_cast<int>(PromptSlot::DINNER)];
    if (!mealReply[0]) mealReply = promptReplies[static_cast<int>(PromptSlot::LUNCH)];
    if (!mealReply[0]) mealReply = promptReplies[static_cast<int>(PromptSlot::BREAKFAST)];

    if (moodReply[0]) {
        const char* tag = classifyPromptReply(PromptSlot::RANDOM_MOOD, moodReply);
        if (strcmp(tag, "happy") == 0) {
            return u8"想到你说开心, 我也摇尾巴";
        }
        if (strcmp(tag, "sad") == 0) {
            return u8"今天也想陪你松一口气";
        }
        if (strcmp(tag, "unsure") == 0) {
            return u8"没关系, 我陪你慢慢想";
        }
    }

    if (lateReply[0]) {
        const char* tag = classifyPromptReply(PromptSlot::LATE_NIGHT, lateReply);
        if (strcmp(tag, "sleep_now") == 0) {
            return u8"想到你会早点睡, 我很安心";
        }
        if (strcmp(tag, "play_more") == 0) {
            return u8"就算想再玩会, 也要慢一点哦";
        }
    }

    if (mealReply[0]) {
        const char* tag = classifyPromptReply(PromptSlot::DINNER, mealReply);
        if (strcmp(tag, "ate") == 0) {
            return u8"记得你好好吃饭, 我会放心";
        }
        if (strcmp(tag, "later") == 0) {
            return u8"别拖太久呀, 肚子会咕咕叫";
        }
    }

    return "";
}

const char* const* Companion::promptSlotChoices(PromptSlot slot) const {
    static char morningChoices[3][24];
    static char breakfastChoices[3][24];
    static char lunchChoices[3][24];
    static char dinnerChoices[3][24];
    static char lateChoices[3][24];
    static char moodChoices[3][24];
    static const char* morningPtrs[3] = {morningChoices[0], morningChoices[1], morningChoices[2]};
    static const char* breakfastPtrs[3] = {breakfastChoices[0], breakfastChoices[1], breakfastChoices[2]};
    static const char* lunchPtrs[3] = {lunchChoices[0], lunchChoices[1], lunchChoices[2]};
    static const char* dinnerPtrs[3] = {dinnerChoices[0], dinnerChoices[1], dinnerChoices[2]};
    static const char* latePtrs[3] = {lateChoices[0], lateChoices[1], lateChoices[2]};
    static const char* moodPtrs[3] = {moodChoices[0], moodChoices[1], moodChoices[2]};

    auto fillChoices = [](const char* key, char target[3][24], const char* const defaults[3]) {
        for (int i = 0; i < 3; ++i) {
            String entry = PetDialogue::option(key, i);
            const char* text = entry.length() ? entry.c_str() : defaults[i];
            strncpy(target[i], text, 23);
            target[i][23] = '\0';
        }
    };

    static const char* morningDefaults[3] = {u8"1 早呀", u8"2 还困", u8"3 先忙"};
    static const char* breakfastDefaults[3] = {u8"1 吃了", u8"2 等会吃", u8"3 还不饿"};
    static const char* lunchDefaults[3] = {u8"1 去吃饭", u8"2 稍后吃", u8"3 还不饿"};
    static const char* dinnerDefaults[3] = {u8"1 吃晚饭", u8"2 等会吃", u8"3 吃过啦"};
    static const char* lateDefaults[3] = {u8"1 这就睡", u8"2 再玩会", u8"3 还不困"};
    static const char* moodDefaults[3] = {u8"1 开心", u8"2 不开心", u8"3 不知道"};

    switch (slot) {
        case PromptSlot::MORNING:
            fillChoices("prompt.choices.morning", morningChoices, morningDefaults);
            return morningPtrs;
        case PromptSlot::BREAKFAST:
            fillChoices("prompt.choices.breakfast", breakfastChoices, breakfastDefaults);
            return breakfastPtrs;
        case PromptSlot::LUNCH:
            fillChoices("prompt.choices.lunch", lunchChoices, lunchDefaults);
            return lunchPtrs;
        case PromptSlot::DINNER:
            fillChoices("prompt.choices.dinner", dinnerChoices, dinnerDefaults);
            return dinnerPtrs;
        case PromptSlot::LATE_NIGHT:
            fillChoices("prompt.choices.late_night", lateChoices, lateDefaults);
            return latePtrs;
        case PromptSlot::RANDOM_MOOD:
            fillChoices("prompt.choices.random_mood", moodChoices, moodDefaults);
            return moodPtrs;
        default:
            fillChoices("prompt.choices.morning", morningChoices, morningDefaults);
            return morningPtrs;
    }
}

void Companion::triggerScheduledPrompt(PromptSlot slot) {
    if (slot == PromptSlot::NONE || townSyncActive || outingActive || activePromptSlot != PromptSlot::NONE) return;
    loadPromptMemory();
    int dayStamp = currentDayStamp();
    if (dayStamp < 0) return;
    if (promptQuietUntil > millis()) return;

    const bool useCard = isPromptCardSlot(slot);
    if (useCard) {
        activePromptSlot = slot;
        promptTextEntryActive = false;
        promptInput[0] = '\0';
        setTemporaryStatus(promptSlotQuestion(slot), 3000);
        promptQuietUntil = millis() + PROMPT_CARD_QUIET_MS;
    } else {
        setTemporaryStatus(promptSlotQuestion(slot), 2800);
        markPromptAsked(slot, dayStamp);
        promptQuietUntil = millis() + PROMPT_STATUS_QUIET_MS;
    }
    switch (slot) {
        case PromptSlot::MORNING:
            pushStoryBeat(u8"\u65e9\u4e0a\u6211\u6709\u8ddf\u4f60\u6253\u62db\u547c");
            break;
        case PromptSlot::BREAKFAST:
            pushStoryBeat(u8"\u65e9\u4e0a\u6211\u60f3\u63d0\u9192\u4f60\u5403\u70b9\u4e1c\u897f");
            break;
        case PromptSlot::LUNCH:
            pushStoryBeat(u8"\u4e2d\u5348\u6211\u6709\u60f3\u8d77\u4f60\u7684\u5348\u996d");
            break;
        case PromptSlot::DINNER:
            pushStoryBeat(u8"\u508d\u665a\u6211\u60f3\u8d77\u4f60\u8be5\u5403\u665a\u996d\u4e86");
            break;
        case PromptSlot::LATE_NIGHT:
            pushStoryBeat(u8"\u591c\u91cc\u6211\u8fd8\u5728\u60f3\u7740\u63d0\u9192\u4f60\u4f11\u606f");
            break;
        case PromptSlot::RANDOM_MOOD:
            pushStoryBeat(u8"\u521a\u624d\u6211\u60f3\u95ee\u95ee\u4f60\u73b0\u5728\u5f00\u5fc3\u5417");
            break;
        default:
            break;
    }
    char detail[96];
    snprintf(detail, sizeof(detail), "prompt:%s", promptSlotMemoryKey(slot));
    PetStorage::appendPetMemoryEvent(petId.c_str(), "prompt", detail);
}

void Companion::answerScheduledPrompt(const char* replyText, bool customReply, const char* feedbackText) {
    if (activePromptSlot == PromptSlot::NONE || !replyText || !replyText[0]) return;
    loadPromptMemory();

    int idx = static_cast<int>(activePromptSlot);
    int dayStamp = currentDayStamp();
    if (idx >= 0 && idx < PetStorage::PROMPT_SLOT_COUNT) {
        promptAskedDayStamp[idx] = dayStamp;
        strncpy(promptReplies[idx], replyText, PetStorage::PROMPT_REPLY_LEN - 1);
        promptReplies[idx][PetStorage::PROMPT_REPLY_LEN - 1] = '\0';
        savePromptMemory();
    }

    char detail[160];
    snprintf(detail, sizeof(detail), "%s\t%s", promptSlotMemoryKey(activePromptSlot), replyText);
    PetStorage::appendPetMemoryEvent(petId.c_str(), customReply ? "reply_custom" : "reply_quick", detail);
    schedulePromptFollowup(activePromptSlot, replyText);

    const char* tag = classifyPromptReply(activePromptSlot, replyText);
    switch (activePromptSlot) {
        case PromptSlot::BREAKFAST:
        case PromptSlot::LUNCH:
        case PromptSlot::DINNER:
            if (strcmp(tag, "ate") == 0) pushStoryBeat(u8"\u4f60\u8bf4\u4eca\u5929\u6709\u597d\u597d\u5403\u996d");
            else if (strcmp(tag, "later") == 0) pushStoryBeat(u8"\u4f60\u8bf4\u7b49\u4f1a\u513f\u518d\u53bb\u5403");
            else if (strcmp(tag, "not_hungry") == 0) pushStoryBeat(u8"\u4f60\u8bf4\u6682\u65f6\u8fd8\u4e0d\u592a\u997f");
            break;
        case PromptSlot::LATE_NIGHT:
            if (strcmp(tag, "sleep_now") == 0) pushStoryBeat(u8"\u4f60\u8bf4\u7b49\u4f1a\u513f\u5c31\u53bb\u7761");
            else if (strcmp(tag, "play_more") == 0) pushStoryBeat(u8"\u4f60\u8bf4\u8fd8\u60f3\u518d\u73a9\u4e00\u4f1a\u513f");
            else if (strcmp(tag, "awake") == 0) pushStoryBeat(u8"\u4f60\u8bf4\u73b0\u5728\u8fd8\u4e0d\u56f0");
            break;
        case PromptSlot::RANDOM_MOOD:
            if (strcmp(tag, "happy") == 0) pushStoryBeat(u8"\u4f60\u8bf4\u521a\u624d\u633a\u5f00\u5fc3");
            else if (strcmp(tag, "sad") == 0) pushStoryBeat(u8"\u4f60\u8bf4\u521a\u624d\u6709\u70b9\u4e0d\u5f00\u5fc3");
            else if (strcmp(tag, "unsure") == 0) pushStoryBeat(u8"\u4f60\u8bf4\u5fc3\u60c5\u6709\u70b9\u8bf4\u4e0d\u4e0a\u6765");
            break;
        default:
            break;
    }

    if (feedbackText && feedbackText[0]) {
        setTemporaryStatus(feedbackText, 2600);
    } else {
        setTemporaryStatus(customReply ? u8"我记住你说的话了" : u8"我记住啦", 2200);
    }
    mood = clampStat(mood + 4);
    bond = clampStat(bond + 5);
    markPetProgressDirty();
    activePromptSlot = PromptSlot::NONE;
    promptTextEntryActive = false;
    promptInput[0] = '\0';
}

void Companion::respondToPromptChoice(int choiceIndex) {
    if (activePromptSlot == PromptSlot::NONE) return;
    const char* const* choices = promptSlotChoices(activePromptSlot);
    const char* storedReply = choices[choiceIndex] + 2;

    if (activePromptSlot == PromptSlot::RANDOM_MOOD) {
        const char* feedbackText = nullptr;
        if (choiceIndex == 0) {
            feedbackText = promptValue("prompt.feedback.random_mood.happy", u8"你开心, 我也开心");
            mood = clampStat(mood + 6);
            bond = clampStat(bond + 5);
        } else if (choiceIndex == 1) {
            static String comfort;
            comfort = PetDialogue::pick("prompt", "feedback.random_mood.sad");
            feedbackText = comfort.length() ? comfort.c_str() : nullptr;
            static const char* comfortLines[] = {
                u8"抱一下, 会慢慢好起来",
                u8"那我陪你待一会儿",
                u8"送你一个小笑话: 喵也会打呼噜"
            };
            if (!feedbackText) feedbackText = comfortLines[random(3)];
            mood = clampStat(mood + 3);
            bond = clampStat(bond + 6);
        } else {
            feedbackText = promptValue("prompt.feedback.random_mood.unsure", u8"那就祝你慢慢开心起来");
            bond = clampStat(bond + 4);
        }
        markPetProgressDirty();
        answerScheduledPrompt(storedReply, false, feedbackText);
        return;
    }

    if (activePromptSlot == PromptSlot::BREAKFAST ||
        activePromptSlot == PromptSlot::LUNCH ||
        activePromptSlot == PromptSlot::DINNER) {
        const char* feedbackText = nullptr;
        if (choiceIndex == 0) feedbackText = promptValue("prompt.feedback.meal.done", u8"那就好, 我会放心");
        else if (choiceIndex == 1) feedbackText = promptValue("prompt.feedback.meal.later", u8"好呀, 等会别忘记");
        else feedbackText = promptValue("prompt.feedback.meal.not_hungry", u8"那也要记得慢慢照顾肚子");
        answerScheduledPrompt(storedReply, false, feedbackText);
        return;
    }

    if (activePromptSlot == PromptSlot::LATE_NIGHT) {
        const char* feedbackText = nullptr;
        if (choiceIndex == 0) feedbackText = promptValue("prompt.feedback.late_night.sleep_now", u8"晚安, 我也安静陪你");
        else if (choiceIndex == 1) feedbackText = promptValue("prompt.feedback.late_night.play_more", u8"那就再玩一小会儿");
        else feedbackText = promptValue("prompt.feedback.late_night.awake", u8"不困也要记得放松眼睛");
        answerScheduledPrompt(storedReply, false, feedbackText);
        return;
    }

    answerScheduledPrompt(storedReply, false);
}

const char* Companion::getActivePromptQuestion() const {
    if (activePromptSlot == PromptSlot::NONE) return "";
    return promptSlotQuestion(activePromptSlot);
}

const char* Companion::getActivePromptChoice(uint8_t index) const {
    if (activePromptSlot == PromptSlot::NONE || index >= 3) return "";
    const char* const* choices = promptSlotChoices(activePromptSlot);
    return choices[index] ? choices[index] : "";
}

bool Companion::answerActivePromptChoice(uint8_t index) {
    if (activePromptSlot == PromptSlot::NONE || index >= 3) return false;
    respondToPromptChoice(index);
    return true;
}

void Companion::dismissActivePrompt() {
    if (activePromptSlot == PromptSlot::NONE) return;
    activePromptSlot = PromptSlot::NONE;
    promptTextEntryActive = false;
    promptInput[0] = '\0';
    promptQuietUntil = millis() + PROMPT_STATUS_QUIET_MS;
    setTemporaryStatus(u8"那我晚点再问你", 1800);
}

void Companion::maybePromptOnInteraction() {
    loadPromptMemory();
    if (activePromptSlot != PromptSlot::NONE) return;
    if (townSyncActive || outingActive || focusModeActive) return;

    int hour = currentHour();
    int dayStamp = currentDayStamp();
    if (dayStamp < 0) return;

    if (hour >= 0 && hour < 5 && canTriggerPrompt(PromptSlot::LATE_NIGHT, dayStamp)) {
        triggerScheduledPrompt(PromptSlot::LATE_NIGHT);
    }
}

void Companion::updateScheduledPrompt() {
    loadPromptMemory();
    if (!promptTimer.tick()) return;
    if (activePromptSlot != PromptSlot::NONE || townSyncActive || outingActive || focusModeActive) return;

    updatePromptFollowup();
    if (activePromptSlot != PromptSlot::NONE) return;

    int hour = currentHour();
    int dayStamp = currentDayStamp();
    if (dayStamp < 0) return;

    if (hour >= 8 && hour < 10 && canTriggerPrompt(PromptSlot::MORNING, dayStamp)) {
        triggerScheduledPrompt(PromptSlot::MORNING);
        return;
    }
    if (hour >= 9 && hour < 11 && canTriggerPrompt(PromptSlot::BREAKFAST, dayStamp)) {
        triggerScheduledPrompt(PromptSlot::BREAKFAST);
        return;
    }
    if (hour >= 12 && hour < 14 && canTriggerPrompt(PromptSlot::LUNCH, dayStamp)) {
        triggerScheduledPrompt(PromptSlot::LUNCH);
        return;
    }
    if (hour >= 18 && hour < 20 && canTriggerPrompt(PromptSlot::DINNER, dayStamp)) {
        triggerScheduledPrompt(PromptSlot::DINNER);
        return;
    }
    if (hour >= 10 && hour < 22 && canTriggerPrompt(PromptSlot::RANDOM_MOOD, dayStamp) && random(100) < 6) {
        triggerScheduledPrompt(PromptSlot::RANDOM_MOOD);
        return;
    }
}

void Companion::schedulePromptFollowup(PromptSlot slot, const char* replyText) {
    int idx = static_cast<int>(slot);
    if (idx < 0 || idx >= PetStorage::PROMPT_SLOT_COUNT || !replyText || !replyText[0]) return;

    promptFollowupAt[idx] = 0;
    promptFollowupDayStamp[idx] = currentDayStamp();

    const char* tag = classifyPromptReply(slot, replyText);
    unsigned long delayMs = 0;
    switch (slot) {
        case PromptSlot::BREAKFAST:
        case PromptSlot::LUNCH:
        case PromptSlot::DINNER:
            if (strcmp(tag, "later") == 0) delayMs = 35UL * 60UL * 1000UL;
            break;
        case PromptSlot::LATE_NIGHT:
            if (strcmp(tag, "play_more") == 0 || strcmp(tag, "awake") == 0) {
                delayMs = 20UL * 60UL * 1000UL;
            }
            break;
        case PromptSlot::RANDOM_MOOD:
            if (strcmp(tag, "sad") == 0 || strcmp(tag, "unsure") == 0) {
                delayMs = 45UL * 60UL * 1000UL;
            }
            break;
        default:
            break;
    }

    if (delayMs > 0) {
        promptFollowupAt[idx] = millis() + delayMs;
    }
}

void Companion::updatePromptFollowup() {
    int dayStamp = currentDayStamp();
    if (dayStamp < 0) return;
    if (promptQuietUntil > millis()) return;

    auto triggerFollowup = [&](PromptSlot slot, const char* text) {
        int idx = static_cast<int>(slot);
        if (idx < 0 || idx >= PetStorage::PROMPT_SLOT_COUNT) return false;
        if (promptFollowupAt[idx] == 0 || millis() < promptFollowupAt[idx]) return false;
        if (promptFollowupDayStamp[idx] != dayStamp) return false;
        setTemporaryStatus(text, 2600);
        promptFollowupAt[idx] = 0;
        promptQuietUntil = millis() + PROMPT_STATUS_QUIET_MS;
        char detail[96];
        snprintf(detail, sizeof(detail), "followup:%s", promptSlotMemoryKey(slot));
        PetStorage::appendPetMemoryEvent(petId.c_str(), "prompt", detail);
        pushStoryBeat(text);
        return true;
    };

    int hour = currentHour();
    const char* mealReply = promptReplies[static_cast<int>(PromptSlot::DINNER)];
    if (!mealReply[0]) mealReply = promptReplies[static_cast<int>(PromptSlot::LUNCH)];
    if (!mealReply[0]) mealReply = promptReplies[static_cast<int>(PromptSlot::BREAKFAST)];

    if (triggerFollowup(PromptSlot::BREAKFAST, promptValue("prompt.followup.breakfast", u8"早餐别拖太久哦"))) return;
    if (hour >= 11 && triggerFollowup(PromptSlot::LUNCH, promptValue("prompt.followup.lunch", u8"记得抽空吃点东西"))) return;
    if (hour >= 18 && triggerFollowup(PromptSlot::DINNER, promptValue("prompt.followup.dinner", u8"晚一点也要记得吃饭"))) return;
    if (hour >= 0 && hour < 5 && triggerFollowup(PromptSlot::LATE_NIGHT, promptValue("prompt.followup.late_night", u8"夜深了, 眼睛也想休息"))) return;

    const char* moodReply = promptReplies[static_cast<int>(PromptSlot::RANDOM_MOOD)];
    if (moodReply[0]) {
        const char* moodTag = classifyPromptReply(PromptSlot::RANDOM_MOOD, moodReply);
        if (strcmp(moodTag, "sad") == 0) {
            if (triggerFollowup(PromptSlot::RANDOM_MOOD, promptValue("prompt.followup.random_mood.sad", u8"如果还闷闷的, 我就在这儿"))) return;
        } else if (strcmp(moodTag, "unsure") == 0) {
            if (triggerFollowup(PromptSlot::RANDOM_MOOD, promptValue("prompt.followup.random_mood.unsure", u8"慢慢来也没关系, 我陪你"))) return;
        }
    }
}

bool Companion::handlePromptKey(char key, bool enter, bool backspace, bool tab) {
    if (activePromptSlot == PromptSlot::NONE) return false;

    const char* const* choices = promptSlotChoices(activePromptSlot);

    if (promptTextEntryActive) {
        if (tab) {
            promptTextEntryActive = false;
            promptInput[0] = '\0';
            return true;
        }
        if (backspace) {
            size_t len = strlen(promptInput);
            if (len > 0) promptInput[len - 1] = '\0';
            return true;
        }
        if (enter) {
            if (promptInput[0]) answerScheduledPrompt(promptInput, true);
            else promptTextEntryActive = false;
            return true;
        }
        if (key == 'r') {
            promptTextEntryActive = false;
            promptInput[0] = '\0';
            return true;
        }
        if (key >= 32 && key <= 126) {
            size_t len = strlen(promptInput);
            if (len < sizeof(promptInput) - 2) {
                promptInput[len] = key;
                promptInput[len + 1] = '\0';
            }
            return true;
        }
        return true;
    }

    if (tab) return true;
    if (key == '1') {
        respondToPromptChoice(0);
        return true;
    }
    if (key == '2') {
        respondToPromptChoice(1);
        return true;
    }
    if (key == '3') {
        respondToPromptChoice(2);
        return true;
    }
    if (key == 'r' || key == 'R') {
        promptTextEntryActive = true;
        promptInput[0] = '\0';
        setTemporaryStatus(u8"输入想说的话吧", 1800);
        return true;
    }
    return false;
}

void Companion::replaceSouvenirs(const char items[][PetStorage::SOUVENIR_ITEM_LEN],
                                 const char notes[][PetStorage::SOUVENIR_NOTE_LEN],
                                 uint8_t count) {
    loadSouvenirs();
    uint8_t capped = count > MAX_SOUVENIR_SLOTS ? MAX_SOUVENIR_SLOTS : count;
    for (uint8_t i = 0; i < MAX_SOUVENIR_SLOTS; i++) {
        souvenirItems[i][0] = '\0';
        souvenirNotes[i][0] = '\0';
    }
    for (uint8_t i = 0; i < capped; i++) {
        strncpy(souvenirItems[i], items[i], sizeof(souvenirItems[i]) - 1);
        souvenirItems[i][sizeof(souvenirItems[i]) - 1] = '\0';
        strncpy(souvenirNotes[i], notes[i], sizeof(souvenirNotes[i]) - 1);
        souvenirNotes[i][sizeof(souvenirNotes[i]) - 1] = '\0';
    }
    souvenirCount = capped;
    for (uint8_t i = 0; i < 3; i++) {
        Config::setSouvenirSlot(i, i < souvenirCount ? souvenirItems[i] : "");
        Config::setSouvenirNoteSlot(i, i < souvenirCount ? souvenirNotes[i] : "");
    }
    Config::setSouvenirCount(souvenirCount > 3 ? 3 : souvenirCount);
    Config::save();
    PetStorage::saveSouvenirs(souvenirItems, souvenirNotes, souvenirCount, MAX_SOUVENIR_SLOTS);
}

void Companion::showActionHelp(unsigned long durationMs) {
    noteUserAttention();
    (void)durationMs;
    helpPanelVisible = !helpPanelVisible;
    if (helpPanelVisible) statsPanelVisible = false;
}

void Companion::toggleStatsPanel() {
    noteUserAttention();
    statsPanelVisible = !statsPanelVisible;
    if (statsPanelVisible) helpPanelVisible = false;
}

void Companion::toggleFocusMode() {
    noteUserAttention();
    maybePromptOnInteraction();
    if (townSyncActive) {
        setTemporaryStatus(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", 1400);
        return;
    }
    if (outingActive) {
        setTemporaryStatus(u8"\u732b\u54aa\u5916\u51fa\u4e2d", 1400);
        return;
    }
    if (activePromptSlot != PromptSlot::NONE) {
        setTemporaryStatus(u8"\u5148\u56de\u7b54\u5c0f\u732b", 1600);
        return;
    }
    if (focusModeActive) {
        focusModeActive = false;
        focusModeStartedAt = 0;
        focusModeEndsAt = 0;
        focusModeLastWarnAt = 0;
        setTemporaryStatus(u8"\u5148\u4f11\u606f\u4e00\u4f1a", 1800);
        showNotification(u8"\u732b\u54aa", u8"\u4e13\u6ce8\u6682\u505c", u8"\u6211\u8fd8\u5728\u8fd9\u91cc");
        return;
    }
    if (energy < 18) {
        setTemporaryStatus(u8"\u5148\u7761\u9971\u4e86\u518d\u4e13\u6ce8", 1800);
        return;
    }
    focusModeActive = true;
    focusModeStartedAt = millis();
    focusModeEndsAt = focusModeStartedAt + FOCUS_DURATION_MS;
    focusModeLastWarnAt = 0;
    setTemporaryStatus(u8"\u5f00\u59cb\u4e13\u6ce8\u4e86", 1800);
    showNotification(u8"\u732b\u54aa", u8"\u4e13\u6ce8\u6a21\u5f0f", u8"\u6211\u966a\u4f60\u4e8c\u5341\u4e94\u5206\u949f");
}

void Companion::handleFocusInterrupt() {
    if (!focusModeActive) return;
    unsigned long now = millis();
    if (focusModeLastWarnAt != 0 && now - focusModeLastWarnAt < FOCUS_WARN_COOLDOWN_MS) return;
    focusModeLastWarnAt = now;
    setTemporaryStatus(u8"\u4e13\u5fc3\u54e6,\u6211\u966a\u7740\u4f60", 1800);
}

void Companion::updatePetNeeds() {
    bool changed = false;

    if (fullnessDecayTimer.tick() && fullness > 0) {
        fullness = clampStat(fullness - 2);
        changed = true;
    }
    if (moodDecayTimer.tick() && mood > 0) {
        int moodDrain = 1;
        if (fullness < 30) moodDrain++;
        if (cleanliness < 35) moodDrain++;
        mood = clampStat(mood - moodDrain);
        changed = true;
    }
    if (state != CompanionState::SLEEP && energyDecayTimer.tick() && energy > 0) {
        energy = clampStat(energy - 1);
        changed = true;
    }
    if (cleanlinessDecayTimer.tick() && cleanliness > 0) {
        int cleanDrain = (mood > 70) ? 1 : 2;
        cleanliness = clampStat(cleanliness - cleanDrain);
        changed = true;
    }
    if (bondDecayTimer.tick() && bond > 0) {
        int neglect = 0;
        if (fullness < 25) neglect++;
        if (mood < 25) neglect++;
        if (energy < 20) neglect++;
        if (cleanliness < 20) neglect++;
        if (neglect > 0) {
            bond = clampStat(bond - neglect);
            changed = true;
        }
    }

    if (state == CompanionState::SLEEP && energy < 100 && sleepRecoverTimer.tick()) {
        energy = clampStat(energy + 2);
        changed = true;
    }

    if (changed) markPetProgressDirty();
    savePetProgress(false);
}

void Companion::updateFocusMode() {
    if (!focusModeActive) return;
    if (millis() < focusModeEndsAt) return;

    focusModeActive = false;
    focusModeStartedAt = 0;
    focusModeEndsAt = 0;
    focusModeLastWarnAt = 0;
    mood = clampStat(mood + 10);
    bond = clampStat(bond + 8);
    energy = clampStat(energy - 2);
    markPetProgressDirty();
    triggerHappy();
    setTemporaryStatus(u8"\u4e13\u6ce8\u5b8c\u6210\u5566", 2200);
    pushStoryBeat(u8"\u521a\u624d\u966a\u4f60\u4e13\u5fc3\u4e86\u4e00\u9635");
    showNotification(u8"\u732b\u54aa", u8"\u4e13\u6ce8\u7ed3\u675f", u8"\u8fd9\u6b21\u4f60\u4eec\u90fd\u5f88\u68d2");
}

void Companion::initStars() {
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = random(10, SCREEN_W - 10);
        stars[i].y = random(5, GROUND_Y - 60);
        stars[i].visible = random(2) == 0;
    }
}

int Companion::currentHour() {
    if (promptTestHour >= 0 && promptTestHour <= 23) return promptTestHour;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return lastKnownHour;
    lastKnownHour = timeinfo.tm_hour;
    return lastKnownHour;
}

int Companion::displayHour() {
    return currentHour();
}

bool Companion::isNightTime() {
    int h = displayHour();
    return h >= 19 || h < 6;
}

void Companion::setPromptTestHour(int hour) {
    if (hour < 0 || hour > 23) return;
    promptTestHour = hour;
    char body[24];
    snprintf(body, sizeof(body), u8"%02d:00", hour);
    showNotification(u8"测试时间", u8"已切换", body);
}

void Companion::clearPromptTestHour() {
    promptTestHour = -1;
    showNotification(u8"测试时间", u8"已关闭", u8"恢复真实时间");
}

void Companion::triggerMoodPromptTest() {
    noteUserAttention();
    triggerScheduledPrompt(PromptSlot::RANDOM_MOOD);
}

void Companion::setState(CompanionState newState) {
    state = newState;
    frameIndex = 0;
    stateStartTime = millis();
    animTimer.reset();

    switch (state) {
        case CompanionState::IDLE:
            animTimer.setInterval(500);
            break;
        case CompanionState::HAPPY:
            animTimer.setInterval(200);
            break;
        case CompanionState::SLEEP:
            animTimer.setInterval(1000);
            break;
        case CompanionState::TALK:
            animTimer.setInterval(250);
            break;
        case CompanionState::STRETCH:
            animTimer.setInterval(400);
            break;
        case CompanionState::LOOK:
            animTimer.setInterval(300);
            break;
    }
}

void Companion::trySpontaneousAction() {
    if (state != CompanionState::IDLE) return;
    if (!spontaneousTimer.tick()) return;

    int action = random(100);
    int happyChance = 12;
    int stretchChance = 24;
    int lookChance = 24;

    if (weather.valid) {
        switch (weather.type) {
            case WeatherType::CLEAR:
            case WeatherType::PARTLY_CLOUDY:
                happyChance += 10;
                stretchChance += 8;
                break;
            case WeatherType::RAIN:
            case WeatherType::DRIZZLE:
            case WeatherType::THUNDER:
                lookChance += 10;
                happyChance -= 4;
                break;
            case WeatherType::FOG:
            case WeatherType::OVERCAST:
                happyChance -= 2;
                stretchChance += 4;
                break;
            default:
                break;
        }
    }

    if (mood > 75) happyChance += 6;
    if (energy < 25) happyChance -= 8;

    int firstCut = max(0, happyChance);
    int secondCut = firstCut + max(0, stretchChance);
    int thirdCut = secondCut + max(0, lookChance);

    if (action < firstCut) {
        triggerHappy();
    } else if (action < secondCut) {
        setState(CompanionState::STRETCH);
    } else if (action < thirdCut) {
        setState(CompanionState::LOOK);
    }

    // Randomize next spontaneous timer
    spontaneousTimer.setInterval(8000 + random(7000));
}

void Companion::tick() {
    // Advance animation frame
    if (animTimer.tick()) {
        frameIndex++;

        switch (state) {
            case CompanionState::IDLE:
                frameIndex %= IDLE_FRAME_COUNT;
                break;
            case CompanionState::HAPPY:
                frameIndex %= HAPPY_FRAME_COUNT;
                if (millis() - stateStartTime > 1200) { // 3 cycles × 2 frames × 200ms
                    setState(CompanionState::IDLE);
                }
                break;
            case CompanionState::SLEEP:
                frameIndex %= SLEEP_FRAME_COUNT;
                break;
            case CompanionState::TALK:
                frameIndex %= TALK_FRAME_COUNT;
                break;
            case CompanionState::STRETCH:
                frameIndex %= HAPPY_FRAME_COUNT;
                if (millis() - stateStartTime > 1600) { // 2 cycles × 2 frames × 400ms
                    setState(CompanionState::IDLE);
                }
                break;
            case CompanionState::LOOK:
                frameIndex %= IDLE_FRAME_COUNT;
                if (millis() - stateStartTime > 2400) { // 2 cycles × 4 frames × 300ms
                    setState(CompanionState::IDLE);
                }
                break;
        }
    }

    // Auto-sleep after idle timeout
    if (state == CompanionState::IDLE && idleTimeout.tick()) {
        setState(CompanionState::SLEEP);
    }

    if (!townSyncActive) {
        updateMoisture();
        updatePetNeeds();
        updateFocusMode();
        updateScheduledPrompt();
        updateToyGame();
        updateOuting();
        trySpontaneousAction();
    }

    // Twinkle stars (only at night)
    if (isNightTime() && starTimer.tick()) {
        int idx = random(MAX_STARS);
        stars[idx].visible = !stars[idx].visible;
    }
}

void Companion::update(M5Canvas& canvas) {
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    tick();

    // Draw everything
    drawBackground(canvas);
    drawCharacter(canvas);
    drawFocusBubble(canvas);
    drawSleepZ(canvas);
    drawToyGame(canvas);
    drawOutingScene(canvas);
    drawTownSyncScene(canvas);
    drawSouvenirViewer(canvas);
    drawClock(canvas);
    drawSimStatusBar(canvas);
    drawStatusText(canvas);
    drawActionBar(canvas);
    drawStatsPanel(canvas);
    drawQuickHint(canvas);
    drawPromptCard(canvas);
}

void Companion::handleKey(char key) {
    if (townSyncActive) return;
    idleTimeout.reset();
    noteUserAttention();

    if (handlePromptKey(key, key == '\n', false, false)) return;
    maybePromptOnInteraction();

    if (state == CompanionState::SLEEP) {
        setState(CompanionState::IDLE);
        playKeyClick();
        return;
    }

    if (key == ' ' || key == '\n') {
        const char* status = personalityAmbient("happy");
        const char* memoryStatus = recentMemoryAmbient("happy");
        if (memoryStatus[0] && random(100) < 28) {
            status = memoryStatus;
        }
        setTemporaryStatus(status, 1600);
        triggerHappy();
    } else {
        playKeyClick();
    }
}

void Companion::feed(bool visualFeedback) {
    noteUserAttention();
    maybePromptOnInteraction();
    if (townSyncActive) {
        setTemporaryStatus(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", 1400);
        return;
    }
    if (outingActive) {
        setTemporaryStatus(u8"\u732b\u54aa\u5916\u51fa\u4e2d", 1400);
        return;
    }
    if (isCooldownActive(lastFeedTime, 45000)) {
        setTemporaryStatus(u8"\u8fd8\u4e0d\u997f\u5462", 1400);
        return;
    }
    if (fullness >= 92) {
        setTemporaryStatus(u8"\u5403\u9971\u5566", 1400);
        return;
    }
    fullness = clampStat(fullness + 22);
    mood = clampStat(mood + 6);
    energy = clampStat(energy + 2);
    cleanliness = clampStat(cleanliness - 1);
    bond = clampStat(bond + 2);
    lastFeedTime = millis();
    markPetProgressDirty();
    PetStorage::appendEventLog("action", "feed");
    pushStoryBeat(u8"\u6211\u521a\u5403\u5230\u4e86\u4e00\u70b9\u597d\u5403\u7684");
    setTemporaryStatus(u8"\u5494\u568f\u5494\u568f");
    if (visualFeedback) {
        triggerHappy();
        showNotification(u8"\u732b\u54aa", u8"\u5403\u70b9\u5fc3", u8"\u9971\u8179\u4e0a\u5347");
    }
}

void Companion::play(bool visualFeedback) {
    noteUserAttention();
    maybePromptOnInteraction();
    if (townSyncActive) {
        setTemporaryStatus(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", 1400);
        return;
    }
    if (outingActive) {
        setTemporaryStatus(u8"\u732b\u54aa\u5916\u51fa\u4e2d", 1400);
        return;
    }
    if (isCooldownActive(lastPlayTime, 60000)) {
        setTemporaryStatus(u8"\u6b47\u4e00\u4f1a\u513f", 1400);
        return;
    }
    if (energy < 18) {
        setTemporaryStatus(u8"\u56f0\u5f97\u4e0d\u73a9", 1600);
        return;
    }
    if (mood >= 94) {
        setTemporaryStatus(u8"\u5df2\u7ecf\u5f88\u5f00\u5fc3", 1400);
        return;
    }
    bool chasedToy = random(100) < 55;
    mood = clampStat(mood + (chasedToy ? 22 : 18));
    fullness = clampStat(fullness - (chasedToy ? 7 : 6));
    energy = clampStat(energy - (chasedToy ? 14 : 12));
    cleanliness = clampStat(cleanliness - (chasedToy ? 9 : 8));
    bond = clampStat(bond + (chasedToy ? 6 : 4));
    lastPlayTime = millis();
    markPetProgressDirty();
    PetStorage::appendEventLog("action", chasedToy ? "play_chase" : "play");
    pushStoryBeat(chasedToy ? u8"\u6211\u521a\u8ffd\u7740\u73a9\u5177\u8dd1\u4e86\u51e0\u5708" : u8"\u6211\u521a\u6492\u6b22\u73a9\u4e86\u4e00\u4f1a\u513f");
    setTemporaryStatus(chasedToy ? u8"\u8ffd\u7740\u73a9\u5177\u8dd1!" : u8"\u6492\u6b22\u5566!");
    if (visualFeedback) {
        triggerHappy();
        showNotification(u8"\u732b\u54aa", u8"\u73a9\u800d\u65f6\u95f4", chasedToy ? u8"\u8ffd\u7740\u73a9\u5177\u8dd1\u4e86\u597d\u51e0\u5708" : u8"\u5fc3\u60c5\u4e0a\u5347");
    }
}

void Companion::nap(bool visualFeedback) {
    noteUserAttention();
    maybePromptOnInteraction();
    if (townSyncActive) {
        setTemporaryStatus(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", 1400);
        return;
    }
    if (outingActive) {
        setTemporaryStatus(u8"\u732b\u54aa\u5916\u51fa\u4e2d", 1400);
        return;
    }
    if (toyGameActive) {
        setTemporaryStatus(u8"\u5148\u73a9\u5b8c\u5427", 1400);
        return;
    }
    if (energy >= 88) {
        setTemporaryStatus(u8"\u8fd8\u4e0d\u56f0\u5462", 1400);
        return;
    }
    energy = clampStat(energy + 24);
    mood = clampStat(mood + 4);
    cleanliness = clampStat(cleanliness - 2);
    bond = clampStat(bond + 1);
    markPetProgressDirty();
    PetStorage::appendEventLog("action", "nap");
    pushStoryBeat(u8"\u6211\u521a\u6253\u4e86\u4e00\u4e2a\u5c0f\u76f9");
    setTemporaryStatus(u8"\u6253\u4e2a\u5c0f\u76f9");
    if (visualFeedback) {
        triggerSleep();
        showNotification(u8"\u732b\u54aa", u8"\u5c0f\u7761\u4e00\u4f1a", u8"\u4f53\u529b\u4e0a\u5347");
    }
}

void Companion::cleanUp(bool visualFeedback) {
    noteUserAttention();
    maybePromptOnInteraction();
    if (townSyncActive) {
        setTemporaryStatus(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", 1400);
        return;
    }
    if (outingActive) {
        setTemporaryStatus(u8"\u732b\u54aa\u5916\u51fa\u4e2d", 1400);
        return;
    }
    if (isCooldownActive(lastCleanTime, 60000)) {
        setTemporaryStatus(u8"\u8fd8\u5f88\u5e72\u51c0", 1400);
        return;
    }
    if (cleanliness >= 92) {
        setTemporaryStatus(u8"\u5df2\u7ecf\u5e72\u51c0", 1400);
        return;
    }
    cleanliness = clampStat(cleanliness + 28);
    mood = clampStat(mood + 8);
    energy = clampStat(energy - 3);
    bond = clampStat(bond + 3);
    lastCleanTime = millis();
    markPetProgressDirty();
    PetStorage::appendEventLog("action", "cleanup");
    pushStoryBeat(u8"\u6211\u73b0\u5728\u88ab\u6536\u62fe\u5f97\u9999\u9999\u7684");
    setTemporaryStatus(u8"\u6d17\u9999\u9999\u5566");
    if (visualFeedback) {
        triggerHappy();
        showNotification(u8"\u732b\u54aa", u8"\u6e05\u7406\u5b8c\u6210", u8"\u722a\u722a\u4eae\u6676\u6676");
    }
}

void Companion::startToyGame() {
    noteUserAttention();
    maybePromptOnInteraction();
    if (townSyncActive) {
        setTemporaryStatus(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", 1400);
        return;
    }
    if (outingActive) {
        setTemporaryStatus(u8"\u732b\u54aa\u5916\u51fa\u4e2d", 1400);
        return;
    }
    if (toyGameActive) {
        setTemporaryStatus(u8"\u73a9\u5177\u5df2\u51fa", 1400);
        return;
    }
    if (isCooldownActive(lastGameTime, 90000)) {
        setTemporaryStatus(u8"\u73a9\u5177\u51b7\u5374", 1400);
        return;
    }
    if (energy < 22) {
        setTemporaryStatus(u8"\u592a\u56f0\u4e0d\u73a9", 1600);
        return;
    }
    toyGameActive = true;
    toyCatchCount = 0;
    toyGameEndsAt = millis() + 20000;
    lastGameTime = millis();
    placeToyTarget();
    PetStorage::appendEventLog("action", "toy_game_start");
    setTemporaryStatus(u8"\u6293\u4f4f\u73a9\u5177!");
    showNotification(u8"\u732b\u54aa", u8"\u8ffd\u73a9\u5177", u8"\u6293\u52303\u6b21");
}

void Companion::startOuting() {
    noteUserAttention();
    maybePromptOnInteraction();
    if (townSyncActive) {
        setTemporaryStatus(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", 1400);
        return;
    }
    if (outingActive) {
        setTemporaryStatus(u8"\u5df2\u7ecf\u51fa\u95e8\u4e86", 1500);
        return;
    }
    if (isCooldownActive(lastOutingTime, 180000)) {
        setTemporaryStatus(u8"\u5148\u5728\u5bb6\u5f85\u4f1a", 1500);
        return;
    }
    if (energy < 28) {
        setTemporaryStatus(u8"\u592a\u56f0\u4e0d\u51fa\u95e8", 1600);
        return;
    }
    outingActive = true;
    outingEndsAt = millis() + (45000 + random(45000));
    lastOutingTime = millis();
    energy = clampStat(energy - 10);
    mood = clampStat(mood + 3);
    markPetProgressDirty();
    PetStorage::appendEventLog("action", "outing_start");
    pushStoryBeat(u8"\u6211\u521a\u51fa\u95e8\u95f2\u901b\u4e86\u4e00\u8d9f");
    setTemporaryStatus(u8"\u51fa\u95e8\u6e9c\u8fbe\u5566");
    showNotification(u8"\u732b\u54aa", u8"\u5916\u51fa\u4e2d", u8"\u5f88\u5feb\u56de\u6765");
}

void Companion::triggerHappy() {
    setState(CompanionState::HAPPY);
    idleTimeout.reset();
    playHappy();
}

void Companion::triggerTalk() {
    setState(CompanionState::TALK);
    idleTimeout.reset();
}

void Companion::triggerIdle() {
    setState(CompanionState::IDLE);
    idleTimeout.reset();
    playNotification();
}

void Companion::triggerSleep() {
    setState(CompanionState::SLEEP);
}

// ── Weather Simulation Mode ──

static const WeatherType SIM_WEATHER_TYPES[] = {
    WeatherType::CLEAR, WeatherType::PARTLY_CLOUDY, WeatherType::OVERCAST,
    WeatherType::FOG, WeatherType::DRIZZLE, WeatherType::RAIN,
    WeatherType::SNOW, WeatherType::THUNDER
};

static const char* SIM_WEATHER_NAMES[] = {
    u8"\u6674", u8"\u591a\u4e91", u8"\u9634", u8"\u96fe",
    u8"\u6bdb\u6bdb\u96e8", u8"\u96e8", u8"\u96ea", u8"\u96f7\u66b4"
};

static_assert(sizeof(SIM_WEATHER_TYPES)/sizeof(SIM_WEATHER_TYPES[0]) ==
              sizeof(SIM_WEATHER_NAMES)/sizeof(SIM_WEATHER_NAMES[0]),
              "SIM_WEATHER_TYPES and SIM_WEATHER_NAMES must have same count");

void Companion::toggleWeatherSim() {
    weatherSimMode = !weatherSimMode;
    if (weatherSimMode) {
        simWeatherIndex = 0;
        simWeatherData.temperature = 25.0f;
        simWeatherData.type = WeatherType::CLEAR;
        simWeatherData.isDay = true;
        simWeatherData.valid = true;
        setWeather(simWeatherData);
    }
    weatherParticlesInit = false; // re-init particles on both enter and exit
}

void Companion::setSimWeatherType(int index) {
    if (index < 1 || index > 8) return;
    simWeatherIndex = index - 1;
    simWeatherData.type = SIM_WEATHER_TYPES[simWeatherIndex];
    simWeatherData.valid = true;
    setWeather(simWeatherData);
    weatherParticlesInit = false; // re-init particles for new weather
}

AccessoryType Companion::getAccessoryForWeather(WeatherType type) {
    switch (type) {
        case WeatherType::CLEAR:
        case WeatherType::PARTLY_CLOUDY:
            return AccessoryType::SUNGLASSES;
        case WeatherType::RAIN:
        case WeatherType::DRIZZLE:
        case WeatherType::THUNDER:
            return AccessoryType::UMBRELLA;
        case WeatherType::SNOW:
            return AccessoryType::SNOW_HAT;
        case WeatherType::FOG:
        case WeatherType::OVERCAST:
            return AccessoryType::MASK;
        default:
            return AccessoryType::NONE;
    }
}

void Companion::move(int dx, int dy) {
    if (townSyncActive) return;
    if (outingActive) return;
    float speedMult = 1.0f;
    if (bond >= 80 && mood >= 70) speedMult *= 1.1f;
    if (energy < 20) speedMult *= 0.6f;
    else if (energy < 35) speedMult *= 0.8f;
    if (mood < 20) speedMult *= 0.85f;
    int step = max(1, (int)(MOVE_STEP * speedMult));

    charX += dx * step;
    charY += dy * step;

    // Clamp to bounds
    const int moveMaxX = moveMaxXForKind(petKind);
    const int moveMaxY = moveMaxYForKind(petKind);
    if (charX < MOVE_MIN_X) charX = MOVE_MIN_X;
    if (charX > moveMaxX) charX = moveMaxX;
    if (charY < MOVE_MIN_Y) charY = MOVE_MIN_Y;
    if (charY > moveMaxY) charY = moveMaxY;

    // Update facing direction
    if (dx < 0) facingLeft = true;
    if (dx > 0) facingLeft = false;

    // Reset idle timeout on movement
    idleTimeout.reset();

    // Wake from sleep
    if (state == CompanionState::SLEEP) {
        setState(CompanionState::IDLE);
    }

    if (toyGameActive) {
        int catCenterX = charX + charDrawWidthForKind(petKind) / 2;
        int catCenterY = charY + charDrawHeightForKind(petKind) / 2;
        int distX = abs(catCenterX - toyTargetX);
        int distY = abs(catCenterY - toyTargetY);
        if (distX <= 18 && distY <= 18) {
            toyCatchCount++;
            mood = clampStat(mood + 10);
            bond = clampStat(bond + 5);
            energy = clampStat(energy - 2);
            markPetProgressDirty();
            triggerHappy();

            if (toyCatchCount >= 3) {
                toyGameActive = false;
                setTemporaryStatus("全都抓到啦");
                showNotification("猫咪", "追玩具", "亲密上升");
            } else {
                setTemporaryStatus("抓到了!", 1200);
                placeToyTarget();
            }
        }
    }
}

float Companion::getNormX() const {
    int moveMaxX = moveMaxXForKind(petKind);
    if (moveMaxX <= MOVE_MIN_X) return 0.5f;
    return (float)(charX - MOVE_MIN_X) / (moveMaxX - MOVE_MIN_X);
}

float Companion::getNormY() const {
    int moveMaxY = moveMaxYForKind(petKind);
    if (moveMaxY <= MOVE_MIN_Y) return 0.5f;
    return (float)(charY - MOVE_MIN_Y) / (moveMaxY - MOVE_MIN_Y);
}

// ── Sound Effects ──

void Companion::playKeyClick() {
    M5DEVICE.Speaker.tone(800, 20, 0);
}

void Companion::playNotification() {
    M5DEVICE.Speaker.tone(1200, 50, 0);
    delay(100);
    M5DEVICE.Speaker.tone(1600, 50, 0);
}

void Companion::playHappy() {
    M5DEVICE.Speaker.tone(1000, 35, 0);
    delay(60);
    M5DEVICE.Speaker.tone(1400, 35, 0);
    delay(60);
    M5DEVICE.Speaker.tone(1800, 50, 0);
}

// ── Drawing ──

// Blend two RGB565 colors: result = a * (1-t) + b * t, t in [0..255]
static uint16_t blendRGB565(uint16_t a, uint16_t b, uint8_t t) {
    uint8_t r1 = (a >> 11) & 0x1F, g1 = (a >> 5) & 0x3F, b1 = a & 0x1F;
    uint8_t r2 = (b >> 11) & 0x1F, g2 = (b >> 5) & 0x3F, b2 = b & 0x1F;
    uint8_t r = r1 + ((int)(r2 - r1) * t / 255);
    uint8_t g = g1 + ((int)(g2 - g1) * t / 255);
    uint8_t bl = b1 + ((int)(b2 - b1) * t / 255);
    return (r << 11) | (g << 5) | bl;
}

void Companion::drawBackground(M5Canvas& canvas) {
    int h = displayHour();
    uint16_t skyColor, groundColor, groundTopColor;

    if (h >= 6 && h < 17) {
        skyColor = SKY_DAY;
        groundColor = GROUND_DAY;
        groundTopColor = GROUND_DAY_TOP;
    } else if (h >= 17 && h < 19) {
        skyColor = SKY_SUNSET;
        groundColor = Color::GROUND;
        groundTopColor = Color::GROUND_TOP;
    } else {
        skyColor = SKY_NIGHT;
        groundColor = Color::GROUND;
        groundTopColor = Color::GROUND_TOP;
    }

    // Weather sky tinting
    bool hideSun = false;
    if (weather.valid) {
        switch (weather.type) {
            case WeatherType::OVERCAST:
                skyColor = blendRGB565(skyColor, rgb565(130, 140, 160), 80);
                hideSun = true;
                break;
            case WeatherType::RAIN:
            case WeatherType::THUNDER:
                skyColor = blendRGB565(skyColor, rgb565(60, 60, 75), 180);
                hideSun = true;
                break;
            case WeatherType::DRIZZLE:
                skyColor = blendRGB565(skyColor, rgb565(90, 90, 105), 140);
                hideSun = true;
                break;
            case WeatherType::SNOW:
                skyColor = blendRGB565(skyColor, rgb565(120, 120, 135), 150);
                hideSun = true;
                break;
            case WeatherType::FOG:
                skyColor = blendRGB565(skyColor, rgb565(140, 140, 145), 170);
                hideSun = true;
                break;
            default:
                break;
        }
    }

    canvas.fillScreen(skyColor);

    // Day elements (hide sun during heavy weather)
    if (h >= 6 && h < 17) {
        if (!hideSun) {
            drawDayElements(canvas);
        } else {
            // Still draw clouds (darker) but no sun
            uint16_t darkCloud = rgb565(150, 150, 160);
            canvas.fillRoundRect(30, 10, 30, 10, 5, darkCloud);
            canvas.fillRoundRect(40, 5, 20, 10, 5, darkCloud);
            canvas.fillRoundRect(120, 15, 28, 9, 4, darkCloud);
            canvas.fillRoundRect(128, 9, 18, 9, 4, darkCloud);
            canvas.fillRoundRect(180, 12, 26, 8, 4, darkCloud);
        }
    } else if (h >= 17 && h < 19) {
        if (!hideSun) {
            canvas.fillCircle(200, GROUND_Y - 10, 12, SUN_COLOR);
            canvas.fillCircle(200, GROUND_Y - 10, 10, rgb565(255, 160, 40));
        }
    }

    // Night: stars + moon (hide in heavy weather)
    if ((h >= 19 || h < 6) && !hideSun) {
        for (int i = 0; i < MAX_STARS; i++) {
            if (stars[i].visible) {
                canvas.drawPixel(stars[i].x, stars[i].y, Color::STAR);
                if (i % 3 == 0) {
                    canvas.drawPixel(stars[i].x - 1, stars[i].y, Color::STAR);
                    canvas.drawPixel(stars[i].x + 1, stars[i].y, Color::STAR);
                    canvas.drawPixel(stars[i].x, stars[i].y - 1, Color::STAR);
                    canvas.drawPixel(stars[i].x, stars[i].y + 1, Color::STAR);
                }
            }
        }
        canvas.fillCircle(30, 20, 10, MOON_COLOR);
        canvas.fillCircle(34, 17, 9, skyColor);
    }

    // Weather particle effects (rain, snow, fog, thunder flash)
    drawWeatherEffects(canvas);

    // Ground
    canvas.fillRect(0, GROUND_Y, SCREEN_W, SCREEN_H - GROUND_Y, groundColor);
    canvas.drawFastHLine(0, GROUND_Y, SCREEN_W, groundTopColor);

    for (int i = 0; i < 8; i++) {
        int gx = (i * 31 + 10) % SCREEN_W;
        canvas.drawPixel(gx, GROUND_Y + 4, groundTopColor);
        canvas.drawPixel(gx + 15, GROUND_Y + 8, groundTopColor);
    }
}

void Companion::initWeatherParticles() {
    for (int i = 0; i < MAX_RAIN; i++) {
        rainDrops[i].x = random(SCREEN_W);
        rainDrops[i].y = random(GROUND_Y);
    }
    for (int i = 0; i < MAX_SNOW; i++) {
        snowflakes[i].x = random(SCREEN_W);
        snowflakes[i].y = random(GROUND_Y);
        snowflakes[i].drift = random(3) - 1; // -1, 0, or 1
    }
    weatherParticlesInit = true;
}

void Companion::drawWeatherEffects(M5Canvas& canvas) {
    if (!weather.valid) return;
    if (!weatherParticlesInit) initWeatherParticles();

    switch (weather.type) {
        case WeatherType::RAIN:
        case WeatherType::DRIZZLE:
        case WeatherType::THUNDER: {
            // Rain drops — vertical lines falling down
            int count = (weather.type == WeatherType::DRIZZLE) ? 8 : MAX_RAIN;
            int speed = (weather.type == WeatherType::DRIZZLE) ? 3 : 5;
            int len = (weather.type == WeatherType::DRIZZLE) ? 3 : 5;
            uint16_t rainColor = rgb565(140, 160, 200);

            for (int i = 0; i < count; i++) {
                rainDrops[i].y += speed;
                if (rainDrops[i].y >= GROUND_Y) {
                    rainDrops[i].y = random(-10, 0);
                    rainDrops[i].x = random(SCREEN_W);
                }
                if (rainDrops[i].y >= 0) {
                    int endY = rainDrops[i].y + len;
                    if (endY > GROUND_Y) endY = GROUND_Y;
                    canvas.drawFastVLine(rainDrops[i].x, rainDrops[i].y, endY - rainDrops[i].y, rainColor);
                }
            }

            // Thunder: flash every 3-5 seconds
            if (weather.type == WeatherType::THUNDER) {
                unsigned long now = millis();
                if (!thunderFlashing && now - lastThunderFlash > 3000 + random(2000)) {
                    thunderFlashing = true;
                    lastThunderFlash = now;
                }
                if (thunderFlashing) {
                    // Flash white for ~50ms (3 frames at 60fps)
                    if (now - lastThunderFlash < 50) {
                        canvas.fillScreen(rgb565(200, 200, 220));
                        // Redraw rain on top of flash
                        for (int i = 0; i < count; i++) {
                            if (rainDrops[i].y >= 0) {
                                int endY = rainDrops[i].y + len;
                                if (endY > GROUND_Y) endY = GROUND_Y;
                                canvas.drawFastVLine(rainDrops[i].x, rainDrops[i].y, endY - rainDrops[i].y, rainColor);
                            }
                        }
                    } else {
                        thunderFlashing = false;
                    }
                }
            }
            break;
        }

        case WeatherType::SNOW: {
            uint16_t snowColor = rgb565(220, 220, 230);
            for (int i = 0; i < MAX_SNOW; i++) {
                snowflakes[i].y += 1;  // Slow fall
                snowflakes[i].x += snowflakes[i].drift;
                // Re-randomize drift occasionally
                if (random(20) == 0) snowflakes[i].drift = random(3) - 1;

                if (snowflakes[i].y >= GROUND_Y) {
                    snowflakes[i].y = random(-5, 0);
                    snowflakes[i].x = random(SCREEN_W);
                }
                // Wrap X
                if (snowflakes[i].x < 0) snowflakes[i].x = SCREEN_W - 1;
                if (snowflakes[i].x >= SCREEN_W) snowflakes[i].x = 0;

                if (snowflakes[i].y >= 0) {
                    canvas.drawPixel(snowflakes[i].x, snowflakes[i].y, snowColor);
                    // Larger flakes for every 3rd
                    if (i % 3 == 0) {
                        canvas.drawPixel(snowflakes[i].x + 1, snowflakes[i].y, snowColor);
                        canvas.drawPixel(snowflakes[i].x, snowflakes[i].y + 1, snowColor);
                    }
                }
            }
            break;
        }

        case WeatherType::FOG: {
            // Semi-transparent fog: scatter gray dots across sky
            uint16_t fogColor = rgb565(160, 160, 165);
            // Deterministic pattern based on frame for slight shimmer
            int offset = (millis() / 200) % 3;
            for (int y = 20 + offset; y < GROUND_Y; y += 4) {
                for (int x = (y % 6); x < SCREEN_W; x += 6) {
                    canvas.drawPixel(x, y, fogColor);
                }
            }
            break;
        }

        default:
            break;
    }
}

void Companion::drawDayElements(M5Canvas& canvas) {
    // Sun
    canvas.fillCircle(200, 18, 10, SUN_COLOR);
    // Sun rays
    for (int i = 0; i < 8; i++) {
        float angle = i * 0.785f; // 45 degree increments
        int x1 = 200 + cos(angle) * 13;
        int y1 = 18 + sin(angle) * 13;
        int x2 = 200 + cos(angle) * 16;
        int y2 = 18 + sin(angle) * 16;
        canvas.drawLine(x1, y1, x2, y2, SUN_COLOR);
    }

    // Clouds
    canvas.fillRoundRect(40, 12, 24, 8, 4, CLOUD_COLOR);
    canvas.fillRoundRect(48, 8, 16, 8, 4, CLOUD_COLOR);

    canvas.fillRoundRect(130, 18, 20, 6, 3, CLOUD_COLOR);
    canvas.fillRoundRect(136, 14, 14, 6, 3, CLOUD_COLOR);
}

void Companion::drawCharacter(M5Canvas& canvas) {
    if (townSyncActive) return;
    if (outingActive) return;
    const uint16_t* frame = nullptr;
    const SpriteSet& sprites = spriteSetForKind(petKind);

    switch (state) {
        case CompanionState::IDLE:
        case CompanionState::LOOK:
            frame = sprites.idle[frameIndex % sprites.idleCount];
            break;
        case CompanionState::HAPPY:
        case CompanionState::STRETCH:
            frame = sprites.happy[frameIndex % sprites.happyCount];
            break;
        case CompanionState::SLEEP:
            frame = sprites.sleep[frameIndex % sprites.sleepCount];
            break;
        case CompanionState::TALK:
            frame = sprites.talk[frameIndex % sprites.talkCount];
            break;
    }

    if (frame) {
        const int drawW = sprites.spriteW * sprites.scale;
        const int drawH = sprites.spriteH * sprites.scale;
        int yOffset = 0;
        int xOffset = 0;

        // Bounce for happy
        if (state == CompanionState::HAPPY && frameIndex % 2 == 0) {
            yOffset = -6;
        }
        // Slight sway for look
        if (state == CompanionState::LOOK) {
            xOffset = (frameIndex % 2 == 0) ? -3 : 3;
        }
        // Slight stretch up
        if (state == CompanionState::STRETCH && frameIndex % 2 == 0) {
            yOffset = -3;
        }

        int drawAreaW = charDrawWidthForKind(petKind);
        int drawAreaH = charDrawHeightForKind(petKind);
        int drawX = charX + xOffset + (drawAreaW - drawW) / 2;
        int drawY = charY + yOffset + (drawAreaH - drawH) / 2;
        drawSprite(canvas, drawX, drawY, frame, sprites.spriteW, sprites.spriteH, sprites.scale, facingLeft);
        drawConditionEffects(canvas, drawX, drawY);
        drawAccessory(canvas, drawX, drawY);
    }
}

void Companion::drawConditionEffects(M5Canvas& canvas, int drawX, int drawY) {
    if (cleanliness < 35) {
        uint16_t dustColor = rgb565(120, 98, 78);
        canvas.fillRect(drawX + 12, drawY + 18, 3, 3, dustColor);
        canvas.fillRect(drawX + 30, drawY + 27, 3, 3, dustColor);
        if (cleanliness < 20) {
            canvas.fillRect(drawX + 21, drawY + 36, 3, 3, dustColor);
            canvas.fillRect(drawX + 39, drawY + 42, 3, 3, dustColor);
        }
    }

    if (energy < 25 && state != CompanionState::SLEEP) {
        uint16_t sleepyColor = rgb565(70, 70, 95);
        canvas.drawFastHLine(drawX + 12, drawY + 19, 10, sleepyColor);
        canvas.drawFastHLine(drawX + 27, drawY + 19, 10, sleepyColor);
    }

    if (mood < 25 && state != CompanionState::HAPPY) {
        uint16_t poutColor = rgb565(60, 40, 40);
        canvas.drawPixel(drawX + 22, drawY + 32, poutColor);
        canvas.drawPixel(drawX + 23, drawY + 33, poutColor);
        canvas.drawPixel(drawX + 24, drawY + 33, poutColor);
        canvas.drawPixel(drawX + 25, drawY + 33, poutColor);
        canvas.drawPixel(drawX + 26, drawY + 32, poutColor);
    }

    if (fullness < 20) {
        uint16_t weakColor = rgb565(255, 220, 140);
        canvas.drawCircle(drawX + 6, drawY + 8, 3, weakColor);
        canvas.drawString("!", drawX + 4, drawY + 2);
    }
}

void Companion::drawSprite(M5Canvas& canvas, int x, int y, const uint16_t* data,
                           int width, int height, int scale, bool flip) {
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            int srcX = flip ? (width - 1 - px) : px;
            uint16_t color = pgm_read_word(&data[py * width + srcX]);
            if (color != Color::TRANSPARENT) {
                canvas.fillRect(x + px * scale, y + py * scale, scale, scale, color);
            }
        }
    }
}

void Companion::drawClock(M5Canvas& canvas) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        canvas.setTextColor(Color::CLOCK_TEXT);
        canvas.setTextSize(1);
        canvas.drawString("--:--", SCREEN_W / 2 - 15, GROUND_Y + 8);
        return;
    }

    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

    canvas.setTextColor(Color::CLOCK_TEXT);
    canvas.setTextSize(2);

    // Layout: center "HH:MM | 7° | 65%" as a whole if weather valid
    char tempStr[8] = {};
    char humStr[8] = {};
    int tempW = 0;
    int humSepW = 0; // second separator + spacing for humidity
    int humW = 0;
    if (weather.valid) {
        int tempInt = (int)roundf(weather.temperature);
        if (tempInt < -99) tempInt = -99;
        if (tempInt > 99) tempInt = 99;
        snprintf(tempStr, sizeof(tempStr), "%d~", tempInt); // ~ as degree placeholder
        tempW = canvas.textWidth(tempStr);

        if (weather.humidity > 0) {
            snprintf(humStr, sizeof(humStr), "%d%%", weather.humidity);
            humW = canvas.textWidth(humStr);
            humSepW = 10 + 1 + 10; // space + separator line + space
        }
    }

    int tw = canvas.textWidth(timeStr);
    int sep = weather.valid ? 10 : 0; // space before separator
    int sepW = weather.valid ? 1 : 0; // separator line width
    int sep2 = weather.valid ? 10 : 0; // space after separator
    int totalW = tw + sep + sepW + sep2 + tempW + humSepW + humW;
    int startX = (SCREEN_W - totalW) / 2;

    canvas.drawString(timeStr, startX, GROUND_Y + 6);

    if (weather.valid) {
        // Draw first separator line (before temperature)
        int sepX = startX + tw + sep;
        canvas.drawFastVLine(sepX, GROUND_Y + 8, 12, Color::STATUS_DIM);

        // Draw temperature number
        char numStr[8];
        int tempInt = (int)roundf(weather.temperature);
        if (tempInt < -99) tempInt = -99;
        if (tempInt > 99) tempInt = 99;
        snprintf(numStr, sizeof(numStr), "%d", tempInt);
        int tempX = sepX + sepW + sep2;
        canvas.drawString(numStr, tempX, GROUND_Y + 6);
        // Draw small ° circle instead of font glyph
        int numW = canvas.textWidth(numStr);
        int degreeEndX = tempX + numW + 6; // after ° symbol
        canvas.drawCircle(tempX + numW + 3, GROUND_Y + 8, 2, Color::CLOCK_TEXT);

        // Draw second separator line + humidity
        if (humStr[0]) {
            int humSepX = degreeEndX + 10;
            canvas.drawFastVLine(humSepX, GROUND_Y + 8, 12, Color::STATUS_DIM);
            int humX = humSepX + 1 + 10;
            canvas.drawString(humStr, humX, GROUND_Y + 6);
        }
    }
}

void Companion::drawSleepZ(M5Canvas& canvas) {
    if (townSyncActive) return;
    if (state != CompanionState::SLEEP) return;

    unsigned long elapsed = millis() - stateStartTime;
    int phase = (elapsed / 600) % 4;
    int drawW = charDrawWidthForKind(petKind);

    canvas.setTextColor(Color::CLOCK_TEXT);

    if (phase >= 1) {
        canvas.setTextSize(1);
        canvas.drawString("z", charX + drawW + 4, charY + 10);
    }
    if (phase >= 2) {
        canvas.setTextSize(1);
        canvas.drawString("Z", charX + drawW + 10, charY);
    }
    if (phase >= 3) {
        canvas.setTextSize(2);
        canvas.drawString("Z", charX + drawW + 16, charY - 12);
    }
}

void Companion::drawStatusText(M5Canvas& canvas) {
    const char* statusStr = "";
    auto useAmbientCategory = [&](const char* category) {
        if (ambientStatusRefreshPending || ambientStatus[0] == '\0') {
            const char* picked = personalityAmbient(category);
            const char* memoryPicked = recentMemoryAmbient(category);
            if (memoryPicked[0]) {
                int odds = 0;
                if (strcmp(category, "idle") == 0) odds = 24;
                else if (strcmp(category, "talk") == 0) odds = 20;
                else if (strcmp(category, "look") == 0) odds = 16;
                else if (strcmp(category, "lonely") == 0) odds = 12;
                if (odds > 0 && random(100) < odds) {
                    picked = memoryPicked;
                }
            }
            strncpy(ambientStatus, picked, sizeof(ambientStatus) - 1);
            ambientStatus[sizeof(ambientStatus) - 1] = '\0';
            strncpy(ambientStatusCategory, category, sizeof(ambientStatusCategory) - 1);
            ambientStatusCategory[sizeof(ambientStatusCategory) - 1] = '\0';
            ambientStatusUntil = millis();
            ambientStatusRefreshPending = false;
        }
        return ambientStatus;
    };

    if (temporaryStatusUntil > millis() && temporaryStatus[0] != '\0') {
        statusStr = temporaryStatus;
    } else if (focusModeActive) {
        statusStr = u8"\u4e13\u6ce8\u4e2d";
    } else if (outingActive) {
        statusStr = u8"\u6b63\u5728\u5916\u51fa";
    } else if (toyGameActive) {
        statusStr = u8"\u5feb\u6293\u73a9\u5177";
    } else if (cleanliness < 20) {
        statusStr = u8"\u60f3\u6d17\u6fa1\u4e86";
    } else if (fullness < 25) {
        statusStr = u8"\u60f3\u5403\u4e1c\u897f";
    } else if (energy < 20) {
        statusStr = u8"\u6709\u70b9\u56f0\u4e86";
    } else if (mood < 25) {
        statusStr = u8"\u60f3\u8981\u73a9\u800d";
    } else if (bond < 20) {
        statusStr = useAmbientCategory("lonely");
    } else if (cleanliness < 40) {
        statusStr = u8"\u722a\u722a\u810f\u4e86";
    } else {
        switch (state) {
            case CompanionState::IDLE:    statusStr = useAmbientCategory("idle"); break;
            case CompanionState::HAPPY:   statusStr = u8"\u55b5\u545c!"; break;
            case CompanionState::SLEEP:   statusStr = u8"\u6b63\u5728\u6253\u76f9"; break;
            case CompanionState::TALK:    statusStr = useAmbientCategory("talk"); break;
            case CompanionState::STRETCH: statusStr = u8"\u4f38\u4e2a\u61d2\u8170"; break;
            case CompanionState::LOOK:    statusStr = useAmbientCategory("look"); break;
        }
    }

    int tabX = SCREEN_W - 60;
    int textW = canvas.textWidth(statusStr);
    int pillW = textW + 10;
    int maxPillW = tabX - 8;
    if (pillW > maxPillW) pillW = maxPillW;
    canvas.fillRoundRect(2, 2, pillW, 12, 4, rgb565(34, 38, 42));
    canvas.drawRoundRect(2, 2, pillW, 12, 4, rgb565(74, 80, 88));
    canvas.setTextColor(rgb565(236, 238, 242));
    canvas.setTextSize(1);
    canvas.drawString(statusStr, 6, 4);

    int dropsX = tabX - 4 - (4 * 7);
    drawBondHearts(canvas, dropsX, 4);
    canvas.setTextColor(Color::STATUS_DIM);
    canvas.setTextSize(1);
    canvas.drawString(u8"[TAB]\u804a\u5929", tabX, 4);
}

void Companion::drawPetMeters(M5Canvas& canvas) {
    const int startX = 4;
    const int startY = 14;
    const int barW = 36;
    const int barH = 4;
    const int gapY = 7;

    struct Meter {
        const char* label;
        uint8_t value;
        uint16_t color;
    } meters[] = {
        { "饱", fullness, rgb565(255, 180, 80) },
        { "乐", mood, rgb565(255, 120, 160) },
        { "力", energy, rgb565(120, 220, 140) },
        { "洁", cleanliness, rgb565(120, 210, 255) },
        { "亲", bond, rgb565(255, 210, 120) },
    };

    canvas.setTextSize(1);
    for (int i = 0; i < 5; i++) {
        int y = startY + i * gapY;
        canvas.setTextColor(Color::STATUS_DIM);
        canvas.drawString(meters[i].label, startX, y - 1);
        canvas.drawRect(startX + 8, y, barW, barH, rgb565(90, 90, 100));
        int fillW = ((barW - 2) * meters[i].value) / 100;
        if (fillW > 0) {
            canvas.fillRect(startX + 9, y + 1, fillW, barH - 2, meters[i].color);
        }
    }
}

void Companion::drawActionBar(M5Canvas& canvas) {
    if (!helpPanelVisible) return;

    const int x = 42;
    const int y = 14;
    const int w = 156;
    const int h = 94;
    uint16_t bg = rgb565(243, 238, 228);
    uint16_t edge = rgb565(140, 124, 102);
    uint16_t line = rgb565(196, 182, 160);
    uint16_t text = Color::BLACK;
    uint16_t dim = rgb565(96, 86, 78);

    canvas.fillRoundRect(x, y, w, h, 8, bg);
    canvas.drawRoundRect(x, y, w, h, 8, edge);
    canvas.drawFastHLine(x + 8, y + 18, w - 16, line);

    canvas.setTextColor(text);
    canvas.setTextSize(1);
    canvas.drawString(u8"\u5e2e\u52a9", x + 62, y + 6);

    canvas.drawString(u8"F \u5582\u98df", x + 10, y + 26);
    canvas.drawString(u8"P \u73a9\u4e50", x + 82, y + 26);
    canvas.drawString(u8"N \u5c0f\u7761", x + 10, y + 38);
    canvas.drawString(u8"C \u6e05\u7406", x + 82, y + 38);
    canvas.drawString(u8"G \u4e13\u6ce8", x + 10, y + 50);
    canvas.drawString(u8"O \u5916\u51fa", x + 82, y + 50);
    canvas.drawString(u8"V \u7eaa\u5ff5", x + 10, y + 62);
    canvas.drawString(u8"T \u8fdb\u57ce", x + 82, y + 62);
    canvas.drawString(u8"I \u72b6\u6001", x + 10, y + 74);
    canvas.drawString(u8"Ctrl+\u56de\u8f66 \u6717\u8bfb", x + 56, y + 74);

    canvas.setTextColor(dim);
    canvas.drawString(u8"[H] \u5173\u95ed", x + 52, y + h - 12);
}

void Companion::drawStatsPanel(M5Canvas& canvas) {
    if (!statsPanelVisible) return;

    const int x = 50;
    const int y = 16;
    const int w = 136;
    const int h = 104;
    canvas.fillRoundRect(x, y, w, h, 8, rgb565(245, 239, 226));
    canvas.drawRoundRect(x, y, w, h, 8, rgb565(140, 124, 102));
    canvas.drawFastHLine(x + 8, y + 18, w - 16, rgb565(196, 182, 160));

    canvas.setTextColor(Color::BLACK);
    canvas.setTextSize(1);
    char panelTitle[32];
    snprintf(panelTitle, sizeof(panelTitle), "%s %s", petName.c_str(), u8"\u72b6\u6001");
    canvas.drawCentreString(panelTitle, x + w / 2, y + 6);

    struct StatRow {
        const char* label;
        uint8_t value;
        uint16_t color;
    } rows[] = {
        { u8"\u9971\u8179", fullness, rgb565(255, 180, 80) },
        { u8"\u5fc3\u60c5", mood, rgb565(255, 120, 160) },
        { u8"\u4f53\u529b", energy, rgb565(120, 220, 140) },
        { u8"\u6e05\u6d01", cleanliness, rgb565(120, 210, 255) },
        { u8"\u4eb2\u5bc6", bond, rgb565(255, 210, 120) },
    };

    for (int i = 0; i < 5; i++) {
        int rowY = y + 24 + i * 12;
        canvas.setTextColor(Color::BLACK);
        canvas.drawString(rows[i].label, x + 8, rowY);
        canvas.drawRect(x + 42, rowY + 2, 58, 6, rgb565(165, 158, 150));
        int fillW = (56 * rows[i].value) / 100;
        if (fillW > 0) {
            canvas.fillRect(x + 43, rowY + 3, fillW, 4, rows[i].color);
        }
        char valueBuf[8];
        snprintf(valueBuf, sizeof(valueBuf), "%3u", rows[i].value);
        canvas.drawString(valueBuf, x + 108, rowY);
    }

    int battery = M5DEVICE.Power.getBatteryLevel();
    bool charging = M5DEVICE.Power.isCharging();
    int infoY = y + 84;
    canvas.setTextColor(Color::BLACK);
    canvas.drawString(u8"\u7535\u91cf", x + 8, infoY);
    char batteryBuf[24];
    if (battery >= 0) {
        snprintf(batteryBuf, sizeof(batteryBuf), "%3d%%%s", battery, charging ? "+" : "");
    } else {
        snprintf(batteryBuf, sizeof(batteryBuf), "--%s", charging ? "+" : "");
    }
    canvas.drawString(batteryBuf, x + 82, infoY);

    canvas.setTextColor(rgb565(96, 86, 78));
    canvas.drawString(u8"[I] \u5173\u95ed", x + 38, y + h - 12);
}

void Companion::drawPromptCard(M5Canvas& canvas) {
    if (activePromptSlot == PromptSlot::NONE) return;
    if (townSyncActive || outingActive) return;

    const int x = 18;
    const int y = 34;
    const int w = SCREEN_W - 36;
    const int h = promptTextEntryActive ? 58 : 74;
    canvas.fillRoundRect(x, y, w, h, 8, rgb565(251, 242, 226));
    canvas.drawRoundRect(x, y, w, h, 8, rgb565(155, 130, 108));
    canvas.drawFastHLine(x + 8, y + 18, w - 16, rgb565(214, 196, 176));
    canvas.setTextColor(Color::BLACK);
    canvas.setTextSize(1);
    canvas.drawString(u8"小猫想问你", x + 10, y + 6);

    const char* question = promptSlotQuestion(activePromptSlot);
    drawSplitNote(canvas, question, x + 10, y + 24, Color::BLACK);

    if (promptTextEntryActive) {
        canvas.setTextColor(rgb565(95, 86, 78));
        canvas.drawString(u8"回车发送 Del删除 R取消", x + 10, y + 46);
        char inputBuf[PetStorage::PROMPT_REPLY_LEN + 2];
        snprintf(inputBuf, sizeof(inputBuf), "> %s_", promptInput);
        canvas.setTextColor(rgb565(36, 44, 60));
        canvas.drawString(inputBuf, x + 10, y + h - 12);
        return;
    }

    const char* const* choices = promptSlotChoices(activePromptSlot);
    canvas.setTextColor(rgb565(36, 44, 60));
    canvas.drawString(choices[0], x + 10, y + 48);
    canvas.drawString(choices[1], x + 72, y + 48);
    canvas.drawString(choices[2], x + 10, y + 60);
    canvas.setTextColor(rgb565(95, 86, 78));
    canvas.drawString(u8"R 自己输入", x + 72, y + 60);
}

void Companion::drawFocusBubble(M5Canvas& canvas) {
    if (!focusModeActive || townSyncActive || outingActive) return;

    unsigned long remainingMs = (focusModeEndsAt > millis()) ? (focusModeEndsAt - millis()) : 0;
    int minutes = (int)(remainingMs / 60000UL);
    int seconds = (int)((remainingMs / 1000UL) % 60UL);
    char text[32];
    snprintf(text, sizeof(text), u8"\u4e13\u6ce8 %02d:%02d", minutes, seconds);

    int bubbleW = canvas.textWidth(text) + 16;
    int bubbleH = 16;
    int bubbleX = charX + (charDrawWidthForKind(petKind) - bubbleW) / 2;
    if (bubbleX < 4) bubbleX = 4;
    if (bubbleX + bubbleW > SCREEN_W - 4) bubbleX = SCREEN_W - 4 - bubbleW;
    int bubbleY = charY - 18;
    if (bubbleY < 18) bubbleY = 18;

    canvas.fillRoundRect(bubbleX, bubbleY, bubbleW, bubbleH, 6, rgb565(247, 240, 224));
    canvas.drawRoundRect(bubbleX, bubbleY, bubbleW, bubbleH, 6, rgb565(145, 128, 104));
    canvas.fillTriangle(bubbleX + bubbleW / 2 - 3, bubbleY + bubbleH,
                        bubbleX + bubbleW / 2 + 3, bubbleY + bubbleH,
                        bubbleX + bubbleW / 2, bubbleY + bubbleH + 4,
                        rgb565(247, 240, 224));
    canvas.drawLine(bubbleX + bubbleW / 2 - 3, bubbleY + bubbleH,
                    bubbleX + bubbleW / 2, bubbleY + bubbleH + 4,
                    rgb565(145, 128, 104));
    canvas.drawLine(bubbleX + bubbleW / 2 + 3, bubbleY + bubbleH,
                    bubbleX + bubbleW / 2, bubbleY + bubbleH + 4,
                    rgb565(145, 128, 104));
    canvas.setTextColor(rgb565(78, 66, 50));
    canvas.drawString(text, bubbleX + 8, bubbleY + 4);
}

void Companion::drawQuickHint(M5Canvas& canvas) {
    if (statsPanelVisible) return;
    if (helpPanelVisible) return;
    if (activePromptSlot != PromptSlot::NONE) return;
    if (outingActive) return;
    if (toyGameActive) return;
    if (souvenirViewerUntil > millis()) return;

    const int w = 64;
    const int h = 10;
    const int x = SCREEN_W - w - 4;
    const int y = SCREEN_H - h - 4;
    uint16_t bg = rgb565(34, 38, 42);
    uint16_t border = rgb565(68, 76, 84);
    uint16_t textColor = rgb565(210, 214, 220);

    canvas.fillRoundRect(x, y, w, h, 3, bg);
    canvas.drawRoundRect(x, y, w, h, 3, border);
    canvas.setTextColor(textColor);
    canvas.setTextSize(1);
    canvas.drawString(u8"H \u5e2e\u52a9 I \u72b6\u6001", x + 3, y + 1);
}

void Companion::drawBondHearts(M5Canvas& canvas, int startX, int y) {
    int heartCount = 0;
    if (bond >= 80) heartCount = 4;
    else if (bond >= 60) heartCount = 3;
    else if (bond >= 40) heartCount = 2;
    else if (bond >= 20) heartCount = 1;

    uint16_t filled = (mood >= 55) ? rgb565(255, 105, 150) : rgb565(190, 120, 150);
    uint16_t dim = rgb565(80, 70, 90);

    for (int i = 0; i < 4; i++) {
        int hx = startX + i * 7;
        uint16_t color = (i < heartCount) ? filled : dim;
        canvas.drawPixel(hx + 1, y + 1, color);
        canvas.drawPixel(hx + 3, y + 1, color);
        canvas.drawFastHLine(hx, y + 2, 5, color);
        canvas.drawFastHLine(hx, y + 3, 5, color);
        canvas.drawFastHLine(hx + 1, y + 4, 3, color);
        canvas.drawPixel(hx + 2, y + 5, color);
    }
}

void Companion::drawToyGame(M5Canvas& canvas) {
    if (townSyncActive) return;
    if (!toyGameActive) return;

    uint16_t toyColor = rgb565(255, 80, 120);
    canvas.fillCircle(toyTargetX, toyTargetY, 4, toyColor);
    canvas.drawCircle(toyTargetX, toyTargetY, 6, Color::WHITE);

    char score[16];
    snprintf(score, sizeof(score), "玩具 %u/3", toyCatchCount);
    canvas.setTextColor(Color::WHITE);
    canvas.setTextSize(1);
    canvas.drawString(score, SCREEN_W - 46, 14);
}

void Companion::drawOutingScene(M5Canvas& canvas) {
    if (!outingActive) return;
    uint16_t bubble = rgb565(245, 236, 210);
    uint16_t edge = rgb565(140, 125, 110);
    int boxX = SCREEN_W / 2 - 42;
    int boxY = MOVE_MIN_Y + 28;
    canvas.fillRoundRect(boxX, boxY, 84, 30, 6, bubble);
    canvas.drawRoundRect(boxX, boxY, 84, 30, 6, edge);
    canvas.setTextColor(Color::BLACK);
    canvas.setTextSize(1);
    canvas.drawString(u8"\u732b\u54aa\u5916\u51fa\u4e2d", boxX + 12, boxY + 7);
    canvas.drawString(u8"\u6b63\u5728\u8857\u533a\u95f2\u901b", boxX + 8, boxY + 17);
}

void Companion::drawTownSyncScene(M5Canvas& canvas) {
    if (!townSyncActive) return;
    uint16_t bubble = rgb565(245, 236, 210);
    uint16_t edge = rgb565(140, 125, 110);
    int boxX = SCREEN_W / 2 - 50;
    int boxY = MOVE_MIN_Y + 28;
    canvas.fillRoundRect(boxX, boxY, 100, 34, 6, bubble);
    canvas.drawRoundRect(boxX, boxY, 100, 34, 6, edge);
    canvas.setTextColor(Color::BLACK);
    canvas.setTextSize(1);
    canvas.drawString(u8"\u5c0f\u732b\u8fdb\u57ce\u4e86", boxX + 12, boxY + 8);
    canvas.drawString(u8"\u7b49\u7535\u8111\u63a5\u8d70\u5b83", boxX + 8, boxY + 20);
}

void Companion::drawSouvenirViewer(M5Canvas& canvas) {
    if (souvenirViewerUntil <= millis() || souvenirCount == 0) return;

    const char* item = souvenirItems[souvenirViewIndex];
    const char* note = souvenirNotes[souvenirViewIndex];
    bool isPhoto = isPhotoSouvenir(item);
    uint16_t card = rgb565(250, 241, 224);
    uint16_t edge = rgb565(155, 130, 108);
    uint16_t muted = rgb565(110, 96, 84);
    uint16_t noteColor = rgb565(88, 74, 58);
    int boxX = 18;
    int boxY = MOVE_MIN_Y + 6;
    int boxW = SCREEN_W - 36;
    int boxH = 86;

    canvas.fillRoundRect(boxX, boxY, boxW, boxH, 8, card);
    canvas.drawRoundRect(boxX, boxY, boxW, boxH, 8, edge);
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextColor(Color::BLACK);
    canvas.setTextSize(1);
    canvas.drawString(u8"纪念盒", boxX + 10, boxY + 7);

    char slot[16];
    snprintf(slot, sizeof(slot), "%u/%u", souvenirViewIndex + 1, souvenirCount);
    canvas.drawRightString(slot, boxX + boxW - 10, boxY + 7);
    canvas.drawFastHLine(boxX + 8, boxY + 18, boxW - 16, rgb565(215, 198, 178));

    if (isPhoto) {
        const char* key = getPhotoKey(item);
        char photoPath[48];
        snprintf(photoPath, sizeof(photoPath), "/pet/photos/%s.png", key);

        int frameX = boxX + 10;
        int frameY = boxY + 26;
        int frameW = 72;
        int frameH = 48;
        canvas.fillRoundRect(frameX - 2, frameY - 2, frameW + 4, frameH + 4, 5, Color::WHITE);
        canvas.drawRoundRect(frameX - 2, frameY - 2, frameW + 4, frameH + 4, 5, edge);

        bool drawOk = false;
        if (PetStorage::isAvailable()) {
            File photoFile = SD.open(photoPath, FILE_READ);
            if (photoFile) {
                constexpr int photoW = 64;
                constexpr int photoH = 48;
                int drawX = frameX + (frameW - photoW) / 2;
                int drawY = frameY + (frameH - photoH) / 2;
                drawOk = canvas.drawPng(&photoFile, drawX, drawY);
                photoFile.close();
            }
        }

        if (!drawOk) {
            canvas.fillRect(frameX, frameY, frameW, frameH, rgb565(210, 220, 225));
            canvas.drawRect(frameX, frameY, frameW, frameH, rgb565(120, 130, 138));
            canvas.setTextColor(rgb565(90, 96, 104));
            canvas.drawCentreString(u8"照片", frameX + frameW / 2, frameY + 14);
            canvas.drawCentreString(u8"读取失败", frameX + frameW / 2, frameY + 28);
            canvas.setTextColor(Color::BLACK);
        }

        int textX = frameX + frameW + 14;
        canvas.drawString(getPhotoLabel(key), textX, frameY + 2);
        drawSplitNote(canvas, note[0] ? note : u8"把风景带回来", textX, frameY + 20, noteColor);
        canvas.setTextColor(muted);
        canvas.drawString(u8"按 V 切换下一页", textX, frameY + 52);
        canvas.setFont(&fonts::efontCN_12);
        return;
    }

    canvas.drawString(item, boxX + 12, boxY + 30);
    drawSplitNote(canvas, note[0] ? note : u8"一点小小纪念", boxX + 12, boxY + 48, noteColor);
    canvas.setTextColor(muted);
    canvas.drawString(u8"按 V 切换下一页", boxX + 12, boxY + 76);
    canvas.setFont(&fonts::efontCN_12);
}




void Companion::placeToyTarget() {
    const int drawW = charDrawWidthForKind(petKind);
    const int drawH = charDrawHeightForKind(petKind);
    const int moveMaxX = moveMaxXForKind(petKind);
    const int moveMaxY = moveMaxYForKind(petKind);
    toyTargetX = random(MOVE_MIN_X + 12, moveMaxX + drawW - 12);
    toyTargetY = random(MOVE_MIN_Y + 12, moveMaxY + drawH - 12);
}

void Companion::updateToyGame() {
    if (!toyGameActive) return;
    if (millis() < toyGameEndsAt) return;
    toyGameActive = false;
    if (toyCatchCount > 0) {
        setTemporaryStatus(u8"\u73a9\u800d\u7ed3\u675f\u5566");
        showNotification(u8"\u732b\u54aa", u8"\u8ffd\u73a9\u5177", u8"\u73a9\u5f97\u4e0d\u9519");
    } else {
        setTemporaryStatus(u8"\u73a9\u5177\u8dd1\u6389\u4e86");
        showNotification(u8"\u732b\u54aa", u8"\u8ffd\u73a9\u5177", u8"\u4e0b\u6b21\u518d\u8bd5");
    }
}

void Companion::updateOuting() {
    if (!outingActive || millis() < outingEndsAt) return;
    static const char* finds[] = {
        u8"\u6811\u53f6\u7167\u7247",
        u8"\u5c0f\u5c0f\u94c3\u94db",
        u8"\u6652\u592a\u9633\u7167",
        u8"\u53e3\u888b\u5c0f\u77f3",
        u8"\u7fbd\u6bdb\u7eb8\u6761",
        u8"\u7a97\u8fb9\u901f\u5199"
    };
    static const char* notes[] = {
        u8"\u98ce\u5f88\u8f7b|\u6211\u5e26\u56de\u4e00\u70b9\u5b89\u9759",
        u8"\u8def\u4e0d\u957f|\u4f46\u5f88\u597d\u8d70",
        u8"\u9633\u5149\u5f88\u6696|\u6bdb\u6bdb\u4e5f\u6696",
        u8"\u7a97\u5916\u4e0b\u96e8|\u5fc3\u91cc\u5f88\u5b89\u5b9a",
        u8"\u6211\u8d70\u5f97\u4e0d\u5feb|\u4f46\u770b\u4e86\u5f88\u4e45",
        u8"\u5c0f\u5df7\u5f88\u5b89\u9759|\u50cf\u8f7b\u8f7b\u6253\u76f9",
        u8"\u4e00\u9635\u98ce\u8fc7\u6765|\u5c3e\u5df4\u5f88\u5f00\u5fc3",
        u8"\u8857\u89d2\u6709\u9999\u5473|\u50cf\u70ed\u70ed\u7684\u996d",
        u8"\u6211\u5750\u4e86\u4e00\u4f1a|\u770b\u5929\u6162\u6162\u53d8\u8272",
        u8"\u4eca\u5929\u7684\u4e91|\u770b\u8d77\u6765\u50cf\u5976\u6cb9",
        u8"\u8def\u8fb9\u7684\u53f6\u5b50|\u50cf\u5c0f\u5c0f\u624b\u638c",
        u8"\u6211\u6ca1\u6709\u8d70\u8fdc|\u53ea\u662f\u60f3\u770b\u770b",
        u8"\u591c\u91cc\u7684\u706f|\u4e00\u9897\u4e00\u9897\u5f88\u6e29\u67d4",
        u8"\u6709\u53ea\u9e1f\u98de\u8fc7|\u6211\u62ac\u5934\u770b\u4e86\u4e00\u4e0b",
        u8"\u5c0f\u5e97\u95e8\u53e3|\u98ce\u94c3\u54cd\u4e86\u4e24\u4e0b",
        u8"\u6d77\u8fb9\u7684\u98ce|\u628a\u8033\u6735\u5439\u5f97\u75d2\u75d2",
        u8"\u6709\u70b9\u60f3\u4f60|\u5c31\u5e26\u70b9\u666f\u8272\u56de\u6765",
        u8"\u4eca\u5929\u6ca1\u4ec0\u4e48\u5927\u4e8b|\u4f46\u5f88\u597d",
        u8"\u6211\u8e29\u8fc7\u6c34\u6d3c|\u5706\u5706\u7684\u5f88\u597d\u770b",
        u8"\u6811\u5f71\u614c\u4e86\u4e00\u4e0b|\u50cf\u5728\u8ddf\u6211\u70b9\u5934",
        u8"\u5e26\u56de\u6765\u7684\u4e0d\u591a|\u4f46\u662f\u6211\u559c\u6b22",
        u8"\u98ce\u505c\u7684\u65f6\u5019|\u4e16\u754c\u4e5f\u6162\u4e86\u4e00\u70b9",
        u8"\u6211\u5728\u62d0\u89d2\u505c\u4e86\u505c|\u95fb\u5230\u4e86\u82b1\u9999",
        u8"\u8fd9\u5f20\u5c0f\u7167\u7247|\u60f3\u7ed9\u4f60\u770b\u770b"
    };
    static const char* photoKeys[] = {
        "roof_sun",
        "window_rain",
        "corner_store",
        "seaside_trip",
        "night_walk",
        "quiet_alley"
    };
    outingActive = false;
    const char* note = notes[random(sizeof(notes) / sizeof(notes[0]))];
    bool gotPhoto = PetStorage::isAvailable() && random(100) < 40;
    char souvenirToken[32];
    const char* notifyText = nullptr;
    if (gotPhoto) {
        const char* key = photoKeys[random(sizeof(photoKeys) / sizeof(photoKeys[0]))];
        snprintf(souvenirToken, sizeof(souvenirToken), "photo:%s", key);
        notifyText = getPhotoLabel(key);
    } else {
        const char* found = finds[random(sizeof(finds) / sizeof(finds[0]))];
        strncpy(souvenirToken, found, sizeof(souvenirToken) - 1);
        souvenirToken[sizeof(souvenirToken) - 1] = 0;
        notifyText = souvenirToken;
    }
    strncpy(lastOutingFind, notifyText, sizeof(lastOutingFind) - 1);
    lastOutingFind[sizeof(lastOutingFind) - 1] = 0;
    pushSouvenir(souvenirToken, note);
    PetStorage::appendEventLog("action", "outing_return");
    pushStoryBeat(gotPhoto ? u8"\u6211\u521a\u5e26\u56de\u6765\u4e00\u5f20\u5c0f\u7167\u7247" : u8"\u6211\u521a\u5e26\u56de\u6765\u4e00\u70b9\u5c0f\u7eaa\u5ff5");
    souvenirViewIndex = 0;
    souvenirViewerUntil = millis() + 5000;
    mood = clampStat(mood + 10);
    bond = clampStat(bond + 6);
    fullness = clampStat(fullness - 4);
    markPetProgressDirty();
    setTemporaryStatus(gotPhoto ? u8"\u5e26\u7167\u7247\u56de\u6765\u4e86" : u8"\u5e26\u793c\u7269\u56de\u6765\u4e86");
    showNotification(u8"\u732b\u54aa", gotPhoto ? u8"\u5e26\u56de\u4e86\u7167\u7247" : u8"\u5df2\u7ecf\u56de\u5bb6", lastOutingFind);
    triggerHappy();
}

// ── Accessories ──

void Companion::drawAccessory(M5Canvas& canvas, int x, int y) {
    (void)canvas;
    (void)x;
    (void)y;
}

void Companion::drawSimStatusBar(M5Canvas& canvas) {
    if (!weatherSimMode) return;

    // Semi-transparent black bar at bottom
    int barY = SCREEN_H - 12;
    canvas.fillRect(0, barY, SCREEN_W, 12, rgb565(20, 20, 20));

    canvas.setTextColor(Color::WHITE);
    canvas.setTextSize(1);

    char label[32];
    snprintf(label, sizeof(label), "[模拟] %s(%d)", SIM_WEATHER_NAMES[simWeatherIndex], simWeatherIndex + 1);
    int tw = canvas.textWidth(label);
    canvas.drawString(label, (SCREEN_W - tw) / 2, barY + 2);
}

// ── Notification Toast ──

void Companion::showNotification(const char* app, const char* title, const char* body) {
    strncpy(notifyApp, app, sizeof(notifyApp) - 1);
    notifyApp[sizeof(notifyApp) - 1] = '\0';
    strncpy(notifyTitle, title, sizeof(notifyTitle) - 1);
    notifyTitle[sizeof(notifyTitle) - 1] = '\0';
    strncpy(notifyBody, body, sizeof(notifyBody) - 1);
    notifyBody[sizeof(notifyBody) - 1] = '\0';
    notificationActive = true;
    notificationStartTime = millis();
    playNotification();
}

void Companion::drawNotificationOverlay(M5Canvas& canvas) {
    if (!notificationActive) return;
    if (millis() - notificationStartTime > NOTIFICATION_DURATION) {
        notificationActive = false;
        return;
    }

    // Dark semi-transparent bar at top
    int barH = 28;
    canvas.fillRect(0, 0, SCREEN_W, barH, rgb565(30, 30, 40));
    canvas.drawFastHLine(0, barH, SCREEN_W, Color::STATUS_DIM);

    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);

    // App name + title on first line
    char line1[80];
    if (notifyApp[0]) {
        snprintf(line1, sizeof(line1), "[%s] %s", notifyApp, notifyTitle);
    } else {
        snprintf(line1, sizeof(line1), "%s", notifyTitle);
    }
    canvas.setTextColor(Color::WHITE);
    canvas.drawString(line1, 4, 2);

    // Body on second line
    canvas.setTextColor(Color::CLOCK_TEXT);
    canvas.drawString(notifyBody, 4, 15);
}

// ── Moisture System ──

void Companion::updateMoisture() {
    if (!weather.valid) return;

    if ((weather.type == WeatherType::CLEAR || weather.type == WeatherType::PARTLY_CLOUDY) && mood < 100 && random(2400) == 0) {
        mood = clampStat(mood + 1);
        markPetProgressDirty();
    } else if ((weather.type == WeatherType::RAIN || weather.type == WeatherType::DRIZZLE || weather.type == WeatherType::THUNDER) && energy > 0 && random(3200) == 0) {
        energy = clampStat(energy - 1);
        markPetProgressDirty();
    } else if ((weather.type == WeatherType::FOG || weather.type == WeatherType::OVERCAST) && mood > 0 && random(3600) == 0) {
        mood = clampStat(mood - 1);
        markPetProgressDirty();
    }
}

void Companion::drawSprayParticles(M5Canvas& canvas) {
    (void)canvas;
}

// ══════════════════════════════════════════════════════════════
// Boot Animation
// ══════════════════════════════════════════════════════════════

void playBootAnimation(M5Canvas& canvas) {
    // Phase 1: Black screen → pixel lobster fades in line by line
    for (int row = 0; row < CHAR_H; row++) {
        canvas.fillScreen(Color::BLACK);

        // Draw revealed rows of the lobster (centered, large)
        int scale = 5;
        int drawW = CHAR_W * scale;
        int drawH = CHAR_H * scale;
        int ox = (SCREEN_W - drawW) / 2;
        int oy = (SCREEN_H - drawH) / 2 - 10;

        for (int py = 0; py <= row; py++) {
            for (int px = 0; px < CHAR_W; px++) {
                uint16_t color = pgm_read_word(&sprite_idle1[py * CHAR_W + px]);
                if (color != Color::TRANSPARENT) {
                    canvas.fillRect(ox + px * scale, oy + py * scale, scale, scale, color);
                }
            }
        }

        canvas.pushSprite(0, 0);
        delay(60);
    }

    // Phase 2: Hold the full logo
    delay(400);

    // Phase 3: Title text appears below
    canvas.setTextColor(Color::CLOCK_TEXT);
    canvas.setTextSize(1);
    int textY = (SCREEN_H + CHAR_H * 5) / 2 - 2;
    canvas.setFont(&fonts::efontCN_12);
    const char* title = u8"\u5c0f\u6a58\u732b";
    int tw = canvas.textWidth(title);
    canvas.drawString(title, (SCREEN_W - tw) / 2, textY);
    canvas.pushSprite(0, 0);
    delay(800);

    // Phase 4: Fade out (darken progressively)
    for (int i = 0; i < 8; i++) {
        canvas.fillRect(0, 0, SCREEN_W, SCREEN_H, canvas.color565(0, 0, 0));
        // Overlay semi-transparent black by drawing translucent pixels
        for (int y = 0; y < SCREEN_H; y += 2) {
            for (int x = (i % 2 == 0 ? 0 : 1); x < SCREEN_W; x += 2) {
                canvas.drawPixel(x, y, Color::BLACK);
            }
        }
        canvas.pushSprite(0, 0);
        delay(60);
    }
    canvas.fillScreen(Color::BLACK);
    canvas.pushSprite(0, 0);
    delay(200);
}

// ══════════════════════════════════════════════════════════════
// Mode Transition Animation
// ══════════════════════════════════════════════════════════════

void playTransition(M5Canvas& canvas, bool toChat) {
    // Slide transition: wipe left (to chat) or right (to companion)
    int dir = toChat ? -1 : 1;

    // Capture isn't possible, so just do a quick pixel wipe
    for (int step = 0; step < 8; step++) {
        int x = step * (SCREEN_W / 8);
        if (toChat) {
            canvas.fillRect(0, 0, x + SCREEN_W / 8, SCREEN_H, Color::BLACK);
        } else {
            canvas.fillRect(SCREEN_W - x - SCREEN_W / 8, 0, x + SCREEN_W / 8, SCREEN_H, Color::BLACK);
        }
        canvas.pushSprite(0, 0);
        delay(25);
    }
}

