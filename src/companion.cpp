#include "companion.h"
#include "sprites.h"
#include "sprites_purple.h"
#include "pet_dialogue.h"
#include <SD.h>
#include <time.h>

// Character draw dimensions
constexpr int CHAR_SCALE = 3;  // 16×3 = 48px on screen
constexpr int CHAR_DRAW_W = CHAR_W * CHAR_SCALE;
constexpr int CHAR_DRAW_H = CHAR_H * CHAR_SCALE;
constexpr int GROUND_Y = SCREEN_H - 28;

// Movement
constexpr int MOVE_STEP = 2;  // 2px per step (~120px/s at 60fps)
constexpr int MOVE_MIN_X = 0;
constexpr int MOVE_MAX_X = SCREEN_W - CHAR_DRAW_W;
constexpr int MOVE_MIN_Y = 16;  // below clock area
constexpr int MOVE_MAX_Y = GROUND_Y - CHAR_DRAW_H - 2;

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
};

static const SpriteSet ORANGE_SET = {
    idle_frames, IDLE_FRAME_COUNT,
    happy_frames, HAPPY_FRAME_COUNT,
    sleep_frames, SLEEP_FRAME_COUNT,
    talk_frames, TALK_FRAME_COUNT
};

static const SpriteSet PURPLE_SET = {
    purple_idle_frames, PURPLE_IDLE_FRAME_COUNT,
    purple_happy_frames, PURPLE_HAPPY_FRAME_COUNT,
    purple_sleep_frames, PURPLE_SLEEP_FRAME_COUNT,
    purple_talk_frames, PURPLE_TALK_FRAME_COUNT
};

static const SpriteSet& spriteSetForKind(const String& kind) {
    if (kind == "purple") return PURPLE_SET;
    return ORANGE_SET;
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
        charX = (SCREEN_W - CHAR_DRAW_W) / 2;
        charY = MOVE_MAX_Y;
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
    ambientStatusCategory[0] = '\0';
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
    charX = newX;
    charY = newY;
    if (charX < MOVE_MIN_X) charX = MOVE_MIN_X;
    if (charX > MOVE_MAX_X) charX = MOVE_MAX_X;
    if (charY < MOVE_MIN_Y) charY = MOVE_MIN_Y;
    if (charY > MOVE_MAX_Y) charY = MOVE_MAX_Y;
    facingLeft = newFacingLeft;
    if (newPetId && newPetId[0]) petId = newPetId;
    if (newPetName && newPetName[0]) petName = newPetName;
    if (newPetKind && newPetKind[0]) petKind = newPetKind;
    if (newPetPersonality && newPetPersonality[0]) petPersonality = normalizePersonality(newPetPersonality);
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

const char* Companion::promptSlotQuestion(PromptSlot slot) const {
    const char* recall = promptSlotRecallQuestion(slot);
    if (recall && recall[0]) return recall;

    switch (slot) {
        case PromptSlot::MORNING: return u8"早安呀|今天想先做什么?";
        case PromptSlot::BREAKFAST: return u8"九点啦|你吃早饭了吗?";
        case PromptSlot::LUNCH: return u8"到中午了|要不要吃点东西?";
        case PromptSlot::DINNER: return u8"傍晚了|晚饭想吃什么?";
        case PromptSlot::LATE_NIGHT: return u8"已经很晚了|要不要早点睡?";
        case PromptSlot::RANDOM_MOOD: return u8"现在心情怎么样?|想和我说说吗?";
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
            if (strcmp(tag, "sleepy") == 0) return u8"早安呀|昨天你说还困, 今天好点了吗?";
            if (strcmp(tag, "busy") == 0) return u8"早安呀|今天也会忙忙的吗?";
            return u8"早安呀|又想先听你说一声早呀";
        case PromptSlot::BREAKFAST:
            if (strcmp(tag, "ate") == 0) return u8"九点啦|上次你有好好吃早饭, 今天也有吗?";
            if (strcmp(tag, "later") == 0) return u8"九点啦|上次你说等会吃, 今天别忘啦";
            if (strcmp(tag, "not_hungry") == 0) return u8"九点啦|今天肚子有比较想吃点东西吗?";
            return u8"九点啦|我又来问问你的早饭";
        case PromptSlot::LUNCH:
            if (strcmp(tag, "ate") == 0) return u8"到中午了|上次你有去吃饭, 今天也要记得哦";
            if (strcmp(tag, "later") == 0) return u8"到中午了|上次你想稍后吃, 今天别拖太久呀";
            if (strcmp(tag, "not_hungry") == 0) return u8"到中午了|现在有没有比刚才更想吃一点?";
            return u8"到中午了|想再确认一下你有没有吃饭";
        case PromptSlot::DINNER:
            if (strcmp(tag, "ate") == 0) return u8"傍晚了|你上次有好好吃晚饭, 今晚也继续吗?";
            if (strcmp(tag, "later") == 0) return u8"傍晚了|这次会不会又想等会再吃呀?";
            if (strcmp(tag, "not_hungry") == 0) return u8"傍晚了|现在还是不太饿吗?";
            return u8"傍晚了|想来问问今晚的肚子";
        case PromptSlot::LATE_NIGHT:
            if (strcmp(tag, "sleep_now") == 0) return u8"已经很晚了|昨晚你有早点睡, 今晚也保持吗?";
            if (strcmp(tag, "play_more") == 0) return u8"已经很晚了|昨晚你还想再玩会, 今晚也一样吗?";
            if (strcmp(tag, "awake") == 0) return u8"已经很晚了|你昨天说还不困, 现在呢?";
            return u8"已经很晚了|今天想早点休息吗?";
        case PromptSlot::RANDOM_MOOD:
            if (strcmp(tag, "happy") == 0) return u8"现在心情怎么样?|上次你说挺开心, 今天也一样吗?";
            if (strcmp(tag, "sad") == 0) return u8"现在心情怎么样?|上次你有点不开心, 今天好一点了吗?";
            if (strcmp(tag, "unsure") == 0) return u8"现在心情怎么样?|上次你说有点说不上来, 今天呢?";
            return u8"现在心情怎么样?|想和我再说说吗?";
        default:
            return "";
    }
}

const char* Companion::recentMemoryAmbient(const char* category) const {
    static char buf[64];
    buf[0] = '\0';

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
    static const char* morningChoices[3] = {u8"1 早呀", u8"2 还困", u8"3 先忙"};
    static const char* breakfastChoices[3] = {u8"1 吃了", u8"2 等会吃", u8"3 还不饿"};
    static const char* lunchChoices[3] = {u8"1 去吃饭", u8"2 稍后吃", u8"3 还不饿"};
    static const char* dinnerChoices[3] = {u8"1 吃晚饭", u8"2 等会吃", u8"3 吃过啦"};
    static const char* lateChoices[3] = {u8"1 这就睡", u8"2 再玩会", u8"3 还不困"};
    static const char* moodChoices[3] = {u8"1 开心", u8"2 不开心", u8"3 不知道"};

    switch (slot) {
        case PromptSlot::MORNING: return morningChoices;
        case PromptSlot::BREAKFAST: return breakfastChoices;
        case PromptSlot::LUNCH: return lunchChoices;
        case PromptSlot::DINNER: return dinnerChoices;
        case PromptSlot::LATE_NIGHT: return lateChoices;
        case PromptSlot::RANDOM_MOOD: return moodChoices;
        default: return morningChoices;
    }
}

void Companion::triggerScheduledPrompt(PromptSlot slot) {
    if (slot == PromptSlot::NONE || townSyncActive || outingActive || activePromptSlot != PromptSlot::NONE) return;
    loadPromptMemory();
    activePromptSlot = slot;
    promptTextEntryActive = false;
    promptInput[0] = '\0';
    setTemporaryStatus(promptSlotQuestion(slot), 3000);
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
            feedbackText = u8"你开心, 我也开心";
            mood = clampStat(mood + 6);
            bond = clampStat(bond + 5);
        } else if (choiceIndex == 1) {
            static const char* comfortLines[] = {
                u8"抱一下, 会慢慢好起来",
                u8"那我陪你待一会儿",
                u8"送你一个小笑话: 喵也会打呼噜"
            };
            feedbackText = comfortLines[random(3)];
            mood = clampStat(mood + 3);
            bond = clampStat(bond + 6);
        } else {
            feedbackText = u8"那就祝你慢慢开心起来";
            bond = clampStat(bond + 4);
        }
        markPetProgressDirty();
        answerScheduledPrompt(storedReply, false, feedbackText);
        return;
    }

    answerScheduledPrompt(storedReply, false);
}

void Companion::maybePromptOnInteraction() {
    loadPromptMemory();
    if (activePromptSlot != PromptSlot::NONE) return;
    if (townSyncActive || outingActive) return;

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
    if (activePromptSlot != PromptSlot::NONE || townSyncActive || outingActive) return;

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
    (void)durationMs;
    helpPanelVisible = !helpPanelVisible;
    if (helpPanelVisible) statsPanelVisible = false;
}

void Companion::toggleStatsPanel() {
    statsPanelVisible = !statsPanelVisible;
    if (statsPanelVisible) helpPanelVisible = false;
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

void Companion::update(M5Canvas& canvas) {
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
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

    // Draw everything
    drawBackground(canvas);
    drawCharacter(canvas);
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

void Companion::feed() {
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
    setTemporaryStatus(u8"\u5494\u568f\u5494\u568f");
    triggerHappy();
    showNotification(u8"\u732b\u54aa", u8"\u5403\u70b9\u5fc3", u8"\u9971\u8179\u4e0a\u5347");
}

void Companion::play() {
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
    mood = clampStat(mood + 18);
    fullness = clampStat(fullness - 6);
    energy = clampStat(energy - 12);
    cleanliness = clampStat(cleanliness - 8);
    bond = clampStat(bond + 4);
    lastPlayTime = millis();
    markPetProgressDirty();
    PetStorage::appendEventLog("action", "play");
    setTemporaryStatus(u8"\u6492\u6b22\u5566!");
    triggerHappy();
    showNotification(u8"\u732b\u54aa", u8"\u73a9\u800d\u65f6\u95f4", u8"\u5fc3\u60c5\u4e0a\u5347");
}

void Companion::nap() {
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
    setTemporaryStatus(u8"\u6253\u4e2a\u5c0f\u76f9");
    triggerSleep();
    showNotification(u8"\u732b\u54aa", u8"\u5c0f\u7761\u4e00\u4f1a", u8"\u4f53\u529b\u4e0a\u5347");
}

void Companion::cleanUp() {
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
    setTemporaryStatus(u8"\u6d17\u9999\u9999\u5566");
    triggerHappy();
    showNotification(u8"\u732b\u54aa", u8"\u6e05\u7406\u5b8c\u6210", u8"\u722a\u722a\u4eae\u6676\u6676");
}

void Companion::startToyGame() {
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
    if (charX < MOVE_MIN_X) charX = MOVE_MIN_X;
    if (charX > MOVE_MAX_X) charX = MOVE_MAX_X;
    if (charY < MOVE_MIN_Y) charY = MOVE_MIN_Y;
    if (charY > MOVE_MAX_Y) charY = MOVE_MAX_Y;

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
        int catCenterX = charX + CHAR_DRAW_W / 2;
        int catCenterY = charY + CHAR_DRAW_H / 2;
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
    if (MOVE_MAX_X <= MOVE_MIN_X) return 0.5f;
    return (float)(charX - MOVE_MIN_X) / (MOVE_MAX_X - MOVE_MIN_X);
}

float Companion::getNormY() const {
    if (MOVE_MAX_Y <= MOVE_MIN_Y) return 0.5f;
    return (float)(charY - MOVE_MIN_Y) / (MOVE_MAX_Y - MOVE_MIN_Y);
}

// ── Sound Effects ──

void Companion::playKeyClick() {
    M5Cardputer.Speaker.tone(800, 30);
}

void Companion::playNotification() {
    M5Cardputer.Speaker.tone(1200, 80);
    delay(100);
    M5Cardputer.Speaker.tone(1600, 80);
}

void Companion::playHappy() {
    M5Cardputer.Speaker.tone(1000, 50);
    delay(60);
    M5Cardputer.Speaker.tone(1400, 50);
    delay(60);
    M5Cardputer.Speaker.tone(1800, 80);
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

        drawSprite16(canvas, charX + xOffset, charY + yOffset, frame, facingLeft);
        drawConditionEffects(canvas, charX + xOffset, charY + yOffset);
        drawAccessory(canvas, charX + xOffset, charY + yOffset);
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

void Companion::drawSprite16(M5Canvas& canvas, int x, int y, const uint16_t* data, bool flip) {
    for (int py = 0; py < CHAR_H; py++) {
        for (int px = 0; px < CHAR_W; px++) {
            int srcX = flip ? (CHAR_W - 1 - px) : px;
            uint16_t color = pgm_read_word(&data[py * CHAR_W + srcX]);
            if (color != Color::TRANSPARENT) {
                canvas.fillRect(x + px * CHAR_SCALE, y + py * CHAR_SCALE,
                               CHAR_SCALE, CHAR_SCALE, color);
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

    canvas.setTextColor(Color::CLOCK_TEXT);

    if (phase >= 1) {
        canvas.setTextSize(1);
        canvas.drawString("z", charX + CHAR_DRAW_W + 4, charY + 10);
    }
    if (phase >= 2) {
        canvas.setTextSize(1);
        canvas.drawString("Z", charX + CHAR_DRAW_W + 10, charY);
    }
    if (phase >= 3) {
        canvas.setTextSize(2);
        canvas.drawString("Z", charX + CHAR_DRAW_W + 16, charY - 12);
    }
}

void Companion::drawStatusText(M5Canvas& canvas) {
    const char* statusStr = "";
    auto useAmbientCategory = [&](const char* category) {
        if (strcmp(ambientStatusCategory, category) != 0 || ambientStatus[0] == '\0') {
            const char* picked = personalityAmbient(category);
            strncpy(ambientStatus, picked, sizeof(ambientStatus) - 1);
            ambientStatus[sizeof(ambientStatus) - 1] = '\0';
            strncpy(ambientStatusCategory, category, sizeof(ambientStatusCategory) - 1);
            ambientStatusCategory[sizeof(ambientStatusCategory) - 1] = '\0';
        }
        return ambientStatus;
    };

    if (temporaryStatusUntil > millis() && temporaryStatus[0] != '\0') {
        statusStr = temporaryStatus;
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

    canvas.setTextColor(Color::STATUS_DIM);
    canvas.setTextSize(1);
    canvas.drawString(statusStr, 4, 4);

    int tabX = SCREEN_W - 60;
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
    canvas.drawString(u8"G \u6e38\u620f", x + 10, y + 50);
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

    int battery = M5Cardputer.Power.getBatteryLevel();
    bool charging = M5Cardputer.Power.isCharging();
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
    toyTargetX = random(MOVE_MIN_X + 12, MOVE_MAX_X + CHAR_DRAW_W - 12);
    toyTargetY = random(MOVE_MIN_Y + 12, MOVE_MAX_Y + CHAR_DRAW_H - 12);
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

