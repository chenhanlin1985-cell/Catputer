#pragma once
#include <M5Cardputer.h>
#include "utils.h"
#include "weather_client.h"
#include "config.h"
#include "pet_storage.h"

enum class CompanionState {
    IDLE,
    HAPPY,
    SLEEP,
    TALK,
    STRETCH,   // spontaneous stretch
    LOOK       // spontaneous look around
};

enum class AccessoryType : uint8_t {
    NONE, SUNGLASSES, UMBRELLA, SNOW_HAT, MASK
};

enum class PromptSlot : int8_t {
    NONE = -1,
    MORNING = 0,
    BREAKFAST = 1,
    LUNCH = 2,
    DINNER = 3,
    LATE_NIGHT = 4,
    RANDOM_MOOD = 5
};

class Companion {
public:
    void begin(M5Canvas& canvas);
    void update(M5Canvas& canvas);
    void handleKey(char key);

    void move(int dx, int dy);  // dx/dy: -1, 0, +1

    // External triggers
    void triggerHappy();
    void triggerTalk();
    void triggerIdle();
    void triggerSleep();

    void setWeather(const WeatherData& wd) { weather = wd; }

    int getHumidityPercent() const { return weather.humidity; }

    // Weather simulation mode
    void toggleWeatherSim();
    void setSimWeatherType(int index); // 1-8
    bool isWeatherSimMode() const { return weatherSimMode; }
    void setPromptTestHour(int hour);
    void clearPromptTestHour();
    uint8_t getFullness() const { return fullness; }
    uint8_t getMood() const { return mood; }
    uint8_t getEnergy() const { return energy; }
    uint8_t getCleanliness() const { return cleanliness; }
    uint8_t getBond() const { return bond; }
    const String& getPetId() const { return petId; }
    const String& getPetName() const { return petName; }
    const String& getPetKind() const { return petKind; }
    const String& getPetPersonality() const { return petPersonality; }
    int getX() const { return charX; }
    int getY() const { return charY; }
    bool isSleeping() const { return state == CompanionState::SLEEP; }
    bool isOuting() const { return outingActive; }
    bool isTownSyncActive() const { return townSyncActive; }
    uint8_t getSouvenirCount() const { return souvenirCount; }
    const char* getSouvenirItem(uint8_t index) const;
    const char* getSouvenirNote(uint8_t index) const;
    void feed();
    void play();
    void nap();
    void cleanUp();
    void startToyGame();
    void startOuting();
    void showSouvenirs();
    void showActionHelp(unsigned long durationMs = 4000);
    void toggleStatsPanel();
    bool handlePromptKey(char key, bool enter, bool backspace, bool tab = false);
    bool hasActivePrompt() const { return activePromptSlot != PromptSlot::NONE; }
    bool isPromptTextEntryActive() const { return promptTextEntryActive; }
    void triggerMoodPromptTest();
    void setTownSyncActive(bool active);
    void applySyncSnapshot(uint8_t newFullness, uint8_t newMood, uint8_t newEnergy,
                           uint8_t newCleanliness, uint8_t newBond, int newX, int newY,
                           bool newFacingLeft, bool sleeping, const char* status,
                           const char* newPetId, const char* newPetName, const char* newPetKind,
                           const char* newPetPersonality);
    void replaceSouvenirs(const char items[][PetStorage::SOUVENIR_ITEM_LEN],
                          const char notes[][PetStorage::SOUVENIR_NOTE_LEN],
                          uint8_t count);
    void exportPromptMemory(int askedDayStamp[PetStorage::PROMPT_SLOT_COUNT],
                            char replies[PetStorage::PROMPT_SLOT_COUNT][PetStorage::PROMPT_REPLY_LEN]);
    void importPromptMemory(const int askedDayStamp[PetStorage::PROMPT_SLOT_COUNT],
                            const char replies[PetStorage::PROMPT_SLOT_COUNT][PetStorage::PROMPT_REPLY_LEN]);

    CompanionState getState() const { return state; }
    WeatherType getWeatherType() const { return weather.type; }
    float getTemperature() const { return weather.temperature; }
    bool hasValidWeather() const { return weather.valid; }
    int getFrameIndex() const { return frameIndex; }
    float getNormX() const;
    float getNormY() const;
    bool isFacingLeft() const { return facingLeft; }

    // Notification toast overlay
    void showNotification(const char* app, const char* title, const char* body);
    void drawNotificationOverlay(M5Canvas& canvas);
    bool hasActiveNotification() const { return notificationActive; }

    // Sound effects
    static void playKeyClick();
    static void playNotification();
    static void playHappy();

private:
    static constexpr uint8_t MAX_SOUVENIR_SLOTS = PetStorage::MAX_SOUVENIRS;
    CompanionState state = CompanionState::IDLE;
    int frameIndex = 0;
    int charX = 0, charY = 0;  // pixel position (initialized in begin())
    bool facingLeft = false;
    Timer animTimer{500};
    Timer idleTimeout{30000};  // 30s → sleep
    Timer clockTimer{1000};
    Timer spontaneousTimer{8000};  // random actions every 8-15s
    unsigned long stateStartTime = 0;
    int lastKnownHour = 20;
    int promptTestHour = -1;

    // Star twinkling
    struct Star { int x, y; bool visible; };
    static constexpr int MAX_STARS = 12;
    Star stars[MAX_STARS];
    Timer starTimer{800};

    // Day/night
    bool isNightTime();
    int currentHour();
    int displayHour();  // time-travel: hour adjusted by pet X position

    void drawBackground(M5Canvas& canvas);
    void drawWeatherEffects(M5Canvas& canvas);
    void drawCharacter(M5Canvas& canvas);
    void drawConditionEffects(M5Canvas& canvas, int drawX, int drawY);
    void drawClock(M5Canvas& canvas);
    void drawSleepZ(M5Canvas& canvas);
    void drawStatusText(M5Canvas& canvas);
    void drawPetMeters(M5Canvas& canvas);
    void drawActionBar(M5Canvas& canvas);
    void drawStatsPanel(M5Canvas& canvas);
    void drawQuickHint(M5Canvas& canvas);
    void drawToyGame(M5Canvas& canvas);
    void drawOutingScene(M5Canvas& canvas);
    void drawTownSyncScene(M5Canvas& canvas);
    void drawSouvenirViewer(M5Canvas& canvas);
    void drawDayElements(M5Canvas& canvas);
    void drawAccessory(M5Canvas& canvas, int charDrawX, int charDrawY);
    void drawBondHearts(M5Canvas& canvas, int startX, int y);
    void drawSimStatusBar(M5Canvas& canvas);
    static AccessoryType getAccessoryForWeather(WeatherType type);

    void setState(CompanionState newState);
    void initStars();
    void trySpontaneousAction();
    void loadPetProgress();
    void savePetProgress(bool force = false);
    void loadSouvenirs();
    void pushSouvenir(const char* item, const char* note);
    void updatePetNeeds();
    void updateScheduledPrompt();
    void updateToyGame();
    void updateOuting();
    void loadPromptMemory();
    void savePromptMemory();
    void markPetProgressDirty();
    void setTemporaryStatus(const char* text, unsigned long durationMs = 2200);
    void placeToyTarget();
    const char* affectionLabel() const;
    String normalizePersonality(const String& value) const;
    const char* personalityAmbient(const char* category) const;
    void maybePromptOnInteraction();
    void triggerScheduledPrompt(PromptSlot slot);
    void answerScheduledPrompt(const char* replyText, bool customReply, const char* feedbackText = nullptr);
    void respondToPromptChoice(int choiceIndex);
    int currentDayStamp() const;
    bool canTriggerPrompt(PromptSlot slot, int dayStamp) const;
    const char* promptSlotQuestion(PromptSlot slot) const;
    const char* const* promptSlotChoices(PromptSlot slot) const;
    const char* promptSlotRecallQuestion(PromptSlot slot) const;
    const char* classifyPromptReply(PromptSlot slot, const char* replyText) const;
    const char* recentMemoryAmbient(const char* category) const;
    void drawPromptCard(M5Canvas& canvas);
    const char* promptSlotMemoryKey(PromptSlot slot) const;

    // Weather simulation
    bool weatherSimMode = false;
    int simWeatherIndex = 0;  // 0-7 for 8 weather types
    WeatherData simWeatherData;

    // Weather state
    WeatherData weather;
    struct RainDrop { int16_t x, y; };
    static constexpr int MAX_RAIN = 15;
    RainDrop rainDrops[MAX_RAIN];
    struct Snowflake { int16_t x, y; int8_t drift; };
    static constexpr int MAX_SNOW = 15;
    Snowflake snowflakes[MAX_SNOW];
    bool weatherParticlesInit = false;
    unsigned long lastThunderFlash = 0;
    bool thunderFlashing = false;

    void initWeatherParticles();

    void updateMoisture();
    void drawSprayParticles(M5Canvas& canvas);

    // Offline pet progression
    bool petProgressLoaded = false;
    bool petProgressDirty = false;
    bool souvenirsLoaded = false;
    uint8_t fullness = 75;
    uint8_t mood = 70;
    uint8_t energy = 80;
    uint8_t cleanliness = 78;
    uint8_t bond = 35;
    String petId = "";
    String petName = "小橘";
    String petKind = "orange";
    String petPersonality = "lively";
    Timer fullnessDecayTimer{300000}; // 5 min
    Timer moodDecayTimer{420000};     // 7 min
    Timer energyDecayTimer{480000};   // 8 min
    Timer cleanlinessDecayTimer{540000}; // 9 min
    Timer bondDecayTimer{1800000};    // 30 min
    Timer sleepRecoverTimer{240000};  // 4 min
    Timer petSaveTimer{15000};        // batch writes to NVS
    unsigned long temporaryStatusUntil = 0;
    char temporaryStatus[24] = "";
    unsigned long ambientStatusUntil = 0;
    char ambientStatus[48] = "";
    char ambientStatusCategory[16] = "";
    bool toyGameActive = false;
    uint8_t toyCatchCount = 0;
    unsigned long toyGameEndsAt = 0;
    int toyTargetX = 0;
    int toyTargetY = 0;
    bool outingActive = false;
    bool townSyncActive = false;
    unsigned long outingEndsAt = 0;
    unsigned long lastOutingTime = 0;
    char lastOutingFind[64] = "";
    char souvenirItems[MAX_SOUVENIR_SLOTS][PetStorage::SOUVENIR_ITEM_LEN] = {{0}};
    char souvenirNotes[MAX_SOUVENIR_SLOTS][PetStorage::SOUVENIR_NOTE_LEN] = {{0}};
    uint8_t souvenirCount = 0;
    uint8_t souvenirViewIndex = 0;
    unsigned long souvenirViewerUntil = 0;
    unsigned long lastFeedTime = 0;
    unsigned long lastPlayTime = 0;
    unsigned long lastCleanTime = 0;
    unsigned long lastGameTime = 0;
    unsigned long actionHelpUntil = 0;
    bool statsPanelVisible = false;
    bool helpPanelVisible = false;
    bool promptMemoryLoaded = false;
    Timer promptTimer{15000};
    PromptSlot activePromptSlot = PromptSlot::NONE;
    bool promptTextEntryActive = false;
    int promptAskedDayStamp[PetStorage::PROMPT_SLOT_COUNT] = {-1, -1, -1, -1, -1};
    char promptReplies[PetStorage::PROMPT_SLOT_COUNT][PetStorage::PROMPT_REPLY_LEN] = {{0}};
    char promptInput[PetStorage::PROMPT_REPLY_LEN] = "";

    // Draw a sprite with transparency (flip=true for horizontal mirror)
    void drawSprite16(M5Canvas& canvas, int x, int y, const uint16_t* data, bool flip = false);

    // Notification overlay state
    bool notificationActive = false;
    unsigned long notificationStartTime = 0;
    static constexpr unsigned long NOTIFICATION_DURATION = 3000;  // 3 seconds
    char notifyApp[32];
    char notifyTitle[48];
    char notifyBody[64];
};

// Boot animation (called from main.cpp)
void playBootAnimation(M5Canvas& canvas);

// Mode transition animation
void playTransition(M5Canvas& canvas, bool toChat);
