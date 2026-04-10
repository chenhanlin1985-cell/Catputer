#pragma once
#include <Arduino.h>

namespace Config {
    // Load saved config from NVS. Returns true if WiFi credentials exist.
    bool load();

    // Save current config to NVS.
    void save();

    // Clear all saved config.
    void reset();

    // Getters
    const String& getSSID();
    const String& getPassword();
    const String& getApiKey();
    const String& getGatewayHost();
    const String& getGatewayPort();
    const String& getGatewayToken();
    const String& getSttHost();
    const String& getSttPort();
    const String& getSSID2();
    const String& getPassword2();
    const String& getGatewayHost2();
    const String& getCity();
    uint8_t getSpeakerVolume();
    bool getAutoSpeak();
    uint8_t getPetFullness();
    uint8_t getPetMood();
    uint8_t getPetEnergy();
    uint8_t getPetCleanliness();
    uint8_t getPetBond();
    const String& getPetId();
    const String& getPetName();
    const String& getPetKind();
    const String& getPetPersonality();
    const String& getSouvenirSlot(uint8_t index);
    const String& getSouvenirNoteSlot(uint8_t index);
    uint8_t getSouvenirCount();

    // Setters
    void setSSID(const String& ssid);
    void setPassword(const String& password);
    void setApiKey(const String& key);
    void setGatewayHost(const String& host);
    void setGatewayPort(const String& port);
    void setGatewayToken(const String& token);
    void setSttHost(const String& host);
    void setSttPort(const String& port);
    void setSSID2(const String& ssid);
    void setPassword2(const String& password);
    void setGatewayHost2(const String& host);
    void setCity(const String& city);
    void setSpeakerVolume(uint8_t volume);
    void setAutoSpeak(bool enabled);
    void setPetFullness(uint8_t value);
    void setPetMood(uint8_t value);
    void setPetEnergy(uint8_t value);
    void setPetCleanliness(uint8_t value);
    void setPetBond(uint8_t value);
    void setPetId(const String& value);
    void setPetName(const String& value);
    void setPetKind(const String& value);
    void setPetPersonality(const String& value);
    void setSouvenirSlot(uint8_t index, const String& value);
    void setSouvenirNoteSlot(uint8_t index, const String& value);
    void setSouvenirCount(uint8_t count);

    // Check if config is valid (has WiFi credentials)
    bool isValid();
}
