#include "config.h"
#include <Preferences.h>

static Preferences prefs;
static String ssid;
static String password;
static String apiKey;
static String gatewayHost;
static String gatewayPort;
static String gatewayToken;
static String sttHost;
static String sttPort;
static String ssid2;
static String password2;
static String gatewayHost2;
static String city;
static uint8_t speakerVolume = 255;
static bool autoSpeak = true;
static bool preferLocalTTS = true;
static uint8_t petFullness = 75;
static uint8_t petMood = 70;
static uint8_t petEnergy = 80;
static uint8_t petCleanliness = 78;
static uint8_t petBond = 35;
static String petId;
static String petName;
static String petKind;
static String petPersonality;
static String souvenirSlots[3];
static String souvenirNoteSlots[3];
static uint8_t souvenirCount = 0;
static bool weatherCacheValid = false;
static float weatherCacheTemperature = 0.0f;
static uint8_t weatherCacheType = 8;
static bool weatherCacheIsDay = true;

bool Config::load() {
    prefs.begin("companion", true); // read-only
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("pass", "");
    apiKey = prefs.getString("apikey", "");
    gatewayHost = prefs.getString("gw_host", "");
    gatewayPort = prefs.getString("gw_port", "");
    gatewayToken = prefs.getString("gw_token", "");
    sttHost = prefs.getString("stt_host", "");
    sttPort = prefs.getString("stt_port", "");
    ssid2 = prefs.getString("ssid2", "");
    password2 = prefs.getString("pass2", "");
    gatewayHost2 = prefs.getString("gw_host2", "");
    city = prefs.getString("city", "");
    speakerVolume = prefs.getUChar("spk_vol", 255);
    autoSpeak = prefs.getBool("auto_tts", true);
    preferLocalTTS = prefs.getBool("local_tts", true);
    petFullness = prefs.getUChar("pet_full", 75);
    petMood = prefs.getUChar("pet_mood", 70);
    petEnergy = prefs.getUChar("pet_energy", 80);
    petCleanliness = prefs.getUChar("pet_clean", 78);
    petBond = prefs.getUChar("pet_bond", 35);
    petId = prefs.getString("pet_id", "");
    petName = prefs.getString("pet_name", "小橘");
    petKind = prefs.getString("pet_kind", "orange");
    petPersonality = prefs.getString("pet_persona", "lively");
    souvenirSlots[0] = prefs.getString("sv_0", "");
    souvenirSlots[1] = prefs.getString("sv_1", "");
    souvenirSlots[2] = prefs.getString("sv_2", "");
    souvenirNoteSlots[0] = prefs.getString("svn_0", "");
    souvenirNoteSlots[1] = prefs.getString("svn_1", "");
    souvenirNoteSlots[2] = prefs.getString("svn_2", "");
    souvenirCount = prefs.getUChar("sv_count", 0);
    weatherCacheValid = prefs.getBool("w_valid", false);
    weatherCacheTemperature = prefs.getFloat("w_temp", 0.0f);
    weatherCacheType = prefs.getUChar("w_type", 8);
    weatherCacheIsDay = prefs.getBool("w_day", true);
    prefs.end();
    return ssid.length() > 0;
}

void Config::save() {
    prefs.begin("companion", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    prefs.putString("apikey", apiKey);
    prefs.putString("gw_host", gatewayHost);
    prefs.putString("gw_port", gatewayPort);
    prefs.putString("gw_token", gatewayToken);
    prefs.putString("stt_host", sttHost);
    prefs.putString("stt_port", sttPort);
    prefs.putString("ssid2", ssid2);
    prefs.putString("pass2", password2);
    prefs.putString("gw_host2", gatewayHost2);
    prefs.putString("city", city);
    prefs.putUChar("spk_vol", speakerVolume);
    prefs.putBool("auto_tts", autoSpeak);
    prefs.putBool("local_tts", preferLocalTTS);
    prefs.putUChar("pet_full", petFullness);
    prefs.putUChar("pet_mood", petMood);
    prefs.putUChar("pet_energy", petEnergy);
    prefs.putUChar("pet_clean", petCleanliness);
    prefs.putUChar("pet_bond", petBond);
    prefs.putString("pet_id", petId);
    prefs.putString("pet_name", petName);
    prefs.putString("pet_kind", petKind);
    prefs.putString("pet_persona", petPersonality);
    prefs.putString("sv_0", souvenirSlots[0]);
    prefs.putString("sv_1", souvenirSlots[1]);
    prefs.putString("sv_2", souvenirSlots[2]);
    prefs.putString("svn_0", souvenirNoteSlots[0]);
    prefs.putString("svn_1", souvenirNoteSlots[1]);
    prefs.putString("svn_2", souvenirNoteSlots[2]);
    prefs.putUChar("sv_count", souvenirCount);
    prefs.putBool("w_valid", weatherCacheValid);
    prefs.putFloat("w_temp", weatherCacheTemperature);
    prefs.putUChar("w_type", weatherCacheType);
    prefs.putBool("w_day", weatherCacheIsDay);
    prefs.end();
}

void Config::reset() {
    prefs.begin("companion", false);
    prefs.clear();
    prefs.end();
    ssid = "";
    password = "";
    apiKey = "";
    gatewayHost = "";
    gatewayPort = "";
    gatewayToken = "";
    sttHost = "";
    sttPort = "";
    ssid2 = "";
    password2 = "";
    gatewayHost2 = "";
    city = "";
    speakerVolume = 255;
    autoSpeak = true;
    preferLocalTTS = true;
    petFullness = 75;
    petMood = 70;
    petEnergy = 80;
    petCleanliness = 78;
    petBond = 35;
    petId = "";
    petName = "小橘";
    petKind = "orange";
    petPersonality = "lively";
    souvenirSlots[0] = "";
    souvenirSlots[1] = "";
    souvenirSlots[2] = "";
    souvenirNoteSlots[0] = "";
    souvenirNoteSlots[1] = "";
    souvenirNoteSlots[2] = "";
    souvenirCount = 0;
    weatherCacheValid = false;
    weatherCacheTemperature = 0.0f;
    weatherCacheType = 8;
    weatherCacheIsDay = true;
}

const String& Config::getSSID() { return ssid; }
const String& Config::getPassword() { return password; }
const String& Config::getApiKey() { return apiKey; }
const String& Config::getGatewayHost() { return gatewayHost; }
const String& Config::getGatewayPort() { return gatewayPort; }
const String& Config::getGatewayToken() { return gatewayToken; }
const String& Config::getSttHost() { return sttHost; }
const String& Config::getSttPort() { return sttPort; }
const String& Config::getSSID2() { return ssid2; }
const String& Config::getPassword2() { return password2; }
const String& Config::getGatewayHost2() { return gatewayHost2; }
const String& Config::getCity() { return city; }
uint8_t Config::getSpeakerVolume() { return speakerVolume; }
bool Config::getAutoSpeak() { return autoSpeak; }
bool Config::getPreferLocalTTS() { return preferLocalTTS; }
uint8_t Config::getPetFullness() { return petFullness; }
uint8_t Config::getPetMood() { return petMood; }
uint8_t Config::getPetEnergy() { return petEnergy; }
uint8_t Config::getPetCleanliness() { return petCleanliness; }
uint8_t Config::getPetBond() { return petBond; }
const String& Config::getPetId() { return petId; }
const String& Config::getPetName() { return petName; }
const String& Config::getPetKind() { return petKind; }
const String& Config::getPetPersonality() { return petPersonality; }
const String& Config::getSouvenirSlot(uint8_t index) {
    static String empty = "";
    if (index >= 3) return empty;
    return souvenirSlots[index];
}
const String& Config::getSouvenirNoteSlot(uint8_t index) {
    static String empty = "";
    if (index >= 3) return empty;
    return souvenirNoteSlots[index];
}
uint8_t Config::getSouvenirCount() { return souvenirCount; }
bool Config::getWeatherCacheValid() { return weatherCacheValid; }
float Config::getWeatherCacheTemperature() { return weatherCacheTemperature; }
uint8_t Config::getWeatherCacheType() { return weatherCacheType; }
bool Config::getWeatherCacheIsDay() { return weatherCacheIsDay; }

void Config::setSSID(const String& s) { ssid = s; }
void Config::setPassword(const String& p) { password = p; }
void Config::setApiKey(const String& k) { apiKey = k; }
void Config::setGatewayHost(const String& h) { gatewayHost = h; }
void Config::setGatewayPort(const String& p) { gatewayPort = p; }
void Config::setGatewayToken(const String& t) { gatewayToken = t; }
void Config::setSttHost(const String& h) { sttHost = h; }
void Config::setSttPort(const String& p) { sttPort = p; }
void Config::setSSID2(const String& s) { ssid2 = s; }
void Config::setPassword2(const String& p) { password2 = p; }
void Config::setGatewayHost2(const String& h) { gatewayHost2 = h; }
void Config::setCity(const String& c) { city = c; }
void Config::setSpeakerVolume(uint8_t v) { speakerVolume = v; }
void Config::setAutoSpeak(bool enabled) { autoSpeak = enabled; }
void Config::setPreferLocalTTS(bool enabled) { preferLocalTTS = enabled; }
void Config::setPetFullness(uint8_t v) { petFullness = v; }
void Config::setPetMood(uint8_t v) { petMood = v; }
void Config::setPetEnergy(uint8_t v) { petEnergy = v; }
void Config::setPetCleanliness(uint8_t v) { petCleanliness = v; }
void Config::setPetBond(uint8_t v) { petBond = v; }
void Config::setPetId(const String& value) { petId = value; }
void Config::setPetName(const String& value) { petName = value; }
void Config::setPetKind(const String& value) { petKind = value; }
void Config::setPetPersonality(const String& value) { petPersonality = value; }
void Config::setSouvenirSlot(uint8_t index, const String& value) {
    if (index >= 3) return;
    souvenirSlots[index] = value;
}
void Config::setSouvenirNoteSlot(uint8_t index, const String& value) {
    if (index >= 3) return;
    souvenirNoteSlots[index] = value;
}
void Config::setSouvenirCount(uint8_t count) { souvenirCount = count > 3 ? 3 : count; }
void Config::setWeatherCache(bool valid, float temperature, uint8_t type, bool isDay) {
    weatherCacheValid = valid;
    weatherCacheTemperature = temperature;
    weatherCacheType = type;
    weatherCacheIsDay = isDay;
}

bool Config::isValid() { return ssid.length() > 0; }
