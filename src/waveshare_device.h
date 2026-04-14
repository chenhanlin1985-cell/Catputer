#pragma once

#if defined(CATPUTER_WAVESHARE_AMOLED_18)

#include <Arduino.h>
#include <Wire.h>
#include <FS.h>
#include <memory>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include <Adafruit_XCA9554.h>
#include <pin_config.h>

namespace fonts {
    extern const void* efontCN_12;
}

class WaveshareCanvas {
public:
    WaveshareCanvas(void* display = nullptr);
    void createSprite(int w, int h);
    void setTextWrap(bool enable);
    void setFont(const void* font);
    void setTextSize(uint8_t size);
    void setTextColor(uint16_t fg);
    void setTextColor(uint16_t fg, uint16_t bg);
    void setCursor(int16_t x, int16_t y);
    void fillScreen(uint16_t color);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color);
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color);
    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color);
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color);
    void drawPixel(int32_t x, int32_t y, uint16_t color);
    void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color);
    void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color);
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
    void drawString(const char* s, int32_t x, int32_t y);
    void drawCentreString(const char* s, int32_t x, int32_t y);
    void drawRightString(const char* s, int32_t right, int32_t y);
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color);
    bool drawPng(File* file, int32_t x, int32_t y);
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) const;
    int textWidth(const char* s) const;
    void pushSprite(int32_t x, int32_t y);

private:
    int width = 0;
    int height = 0;
    uint8_t textSize = 1;
};

using M5Canvas = WaveshareCanvas;

struct WaveshareTouchDetail {
    int32_t x = 0;
    int32_t y = 0;
    bool pressed = false;
    bool pressedEdge = false;
    bool releasedEdge = false;
    bool wasPressed() const { return pressedEdge; }
    bool wasReleased() const { return releasedEdge; }
};

class WaveshareDeviceClass {
public:
    struct Config {};

    struct DisplayClass {
        void setRotation(uint8_t rotation);
        void setBrightness(uint8_t brightness);
    } Display;

    struct SpeakerClass {
        void begin();
        void end();
        void setVolume(uint8_t volume);
        void setChannelVolume(uint8_t, uint8_t) {}
        void tone(uint16_t frequency, uint32_t durationMs, uint8_t channel = 0);
        void playRaw(int16_t* samples, size_t sampleCount, uint32_t sampleRate, bool stereo, uint8_t, uint8_t, bool);
        bool isPlaying() const;
        void stop();
    } Speaker;

    struct MicClass {
        struct Config {
            uint32_t sample_rate = 16000;
            uint8_t magnification = 0;
            uint8_t noise_filter_level = 0;
            uint8_t task_priority = 0;
        };
        Config config() const { return cfg; }
        void config(const Config& c) { cfg = c; }
        void begin() {}
        void end() {}
        size_t record(int16_t* buffer, size_t maxSamples, uint32_t) {
            if (!buffer) return 0;
            for (size_t i = 0; i < maxSamples; ++i) buffer[i] = 0;
            return maxSamples;
        }
    private:
        Config cfg = {};
    } Mic;

    struct PowerClass {
        int getBatteryLevel() const { return 100; }
        bool isCharging() const { return false; }
    } Power;

    struct TouchClass {
        WaveshareTouchDetail getDetail() const { return detail; }
    private:
        friend class WaveshareDeviceClass;
        WaveshareTouchDetail detail = {};
    } Touch;

    Config config() const { return {}; }
    void begin(const Config& cfg);
    void update();
    bool sampleTouch(int32_t& x, int32_t& y, bool& pressed);
    bool initialized() const { return inited; }

private:
    bool inited = false;
    bool displayReady = false;
    bool touchReady = false;
    bool touchPressed = false;
    int32_t lastTouchX = LCD_WIDTH / 2;
    int32_t lastTouchY = LCD_HEIGHT / 2;
    uint32_t lastTouchMs = 0;

    Adafruit_XCA9554 expander;
    std::shared_ptr<Arduino_IIC_DriveBus> i2cBus;
    std::unique_ptr<Arduino_IIC> ft3168;
    Arduino_DataBus* bus = nullptr;
    Arduino_SH8601* gfx = nullptr;

    void initDisplay();
    void initTouch();
};

extern WaveshareDeviceClass M5;

#endif
