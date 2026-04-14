#include "waveshare_device.h"

#if defined(CATPUTER_WAVESHARE_AMOLED_18)

#include "ESP_I2S.h"
#include "../_vendor/ESP32-S3-Touch-AMOLED-1.8/examples/Arduino-v3.3.5/examples/15_ES8311/es8311.h"

namespace fonts {
const void* efontCN_12 =
#if defined(U8G2_FONT_SUPPORT)
    u8g2_font_unifont_h_chinese4;
#else
    nullptr;
#endif
}

static Arduino_GFX* g_gfx = nullptr;
static Arduino_SH8601* g_sh8601 = nullptr;
static Arduino_Canvas* g_canvas = nullptr;
static Arduino_IIC* g_touch = nullptr;
static volatile bool g_touchIrq = false;
static I2SClass g_audioI2s;
static es8311_handle_t g_es8311 = nullptr;
static bool g_audioReady = false;
static uint8_t g_audioVolume = 85;
static uint32_t g_audioPlayingUntilMs = 0;

static bool initEs8311Codec() {
    if (g_es8311) return true;
    g_es8311 = es8311_create(0, ES8311_ADDRRES_0);
    if (!g_es8311) {
        Serial.println("[AUDIO] es8311 create failed");
        return false;
    }

    const es8311_clock_config_t clockConfig = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = 16000 * 256,
        .sample_frequency = 16000,
    };

    if (es8311_init(g_es8311, &clockConfig, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK ||
        es8311_sample_frequency_config(g_es8311, clockConfig.mclk_frequency, clockConfig.sample_frequency) != ESP_OK ||
        es8311_microphone_config(g_es8311, false) != ESP_OK ||
        es8311_microphone_gain_set(g_es8311, ES8311_MIC_GAIN_12DB) != ESP_OK ||
        es8311_voice_volume_set(g_es8311, g_audioVolume, nullptr) != ESP_OK) {
        Serial.println("[AUDIO] es8311 init failed");
        es8311_delete(g_es8311);
        g_es8311 = nullptr;
        return false;
    }
    return true;
}

static bool ensureAudioReady() {
    if (g_audioReady) return true;
    Wire.begin(IIC_SDA, IIC_SCL);
    pinMode(PA, OUTPUT);
    digitalWrite(PA, HIGH);

    g_audioI2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
    if (!g_audioI2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
        Serial.println("[AUDIO] i2s init failed");
        return false;
    }
    if (!initEs8311Codec()) {
        g_audioI2s.end();
        return false;
    }
    g_audioReady = true;
    Serial.println("[AUDIO] es8311 speaker ready");
    return true;
}

static void touchInterruptHandler() {
    g_touchIrq = true;
    if (g_touch) {
        g_touch->IIC_Interrupt_Flag = true;
    }
}

static Arduino_GFX* drawTarget() {
    return g_canvas ? static_cast<Arduino_GFX*>(g_canvas) : g_gfx;
}

WaveshareCanvas::WaveshareCanvas(void*) {}

void WaveshareCanvas::createSprite(int w, int h) {
    width = w;
    height = h;
    if (!g_gfx) return;
    if (g_canvas) {
        delete g_canvas;
        g_canvas = nullptr;
    }
    g_canvas = new Arduino_Canvas(w, h, g_gfx, 0, 0, 0);
    if (g_canvas) {
        bool ok = g_canvas->begin(GFX_SKIP_OUTPUT_BEGIN);
        if (!ok) {
            delete g_canvas;
            g_canvas = nullptr;
        }
    }
}

void WaveshareCanvas::setTextWrap(bool enable) { auto t = drawTarget(); if (t) t->setTextWrap(enable); }
void WaveshareCanvas::setFont(const void*) {
    auto t = drawTarget();
    if (!t) return;
#if defined(U8G2_FONT_SUPPORT)
    t->setUTF8Print(true);
    t->setFont(u8g2_font_unifont_h_chinese4);
#else
    t->setFont(nullptr);
#endif
}
void WaveshareCanvas::setTextSize(uint8_t size) {
    uint8_t scaled = size == 0 ? 1 : size;
    if (LCD_HEIGHT >= 400 && scaled < 4) {
        scaled = scaled * 2;
    }
    textSize = scaled;
    auto t = drawTarget();
    if (t) t->setTextSize(textSize);
}
void WaveshareCanvas::setTextColor(uint16_t fg) { auto t = drawTarget(); if (t) t->setTextColor(fg); }
void WaveshareCanvas::setTextColor(uint16_t fg, uint16_t bg) { auto t = drawTarget(); if (t) t->setTextColor(fg, bg); }
void WaveshareCanvas::setCursor(int16_t x, int16_t y) { auto t = drawTarget(); if (t) t->setCursor(x, y); }
void WaveshareCanvas::fillScreen(uint16_t color) { auto t = drawTarget(); if (t) t->fillScreen(color); }
void WaveshareCanvas::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) { auto t = drawTarget(); if (t) t->fillRect(x, y, w, h, color); }
void WaveshareCanvas::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) { auto t = drawTarget(); if (t) t->drawRect(x, y, w, h, color); }
void WaveshareCanvas::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) { auto t = drawTarget(); if (t) t->fillRoundRect(x, y, w, h, r, color); }
void WaveshareCanvas::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) { auto t = drawTarget(); if (t) t->drawRoundRect(x, y, w, h, r, color); }
void WaveshareCanvas::drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) { auto t = drawTarget(); if (t) t->drawFastHLine(x, y, w, color); }
void WaveshareCanvas::drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) { auto t = drawTarget(); if (t) t->drawFastVLine(x, y, h, color); }
void WaveshareCanvas::drawPixel(int32_t x, int32_t y, uint16_t color) { auto t = drawTarget(); if (t) t->drawPixel(x, y, color); }
void WaveshareCanvas::drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) { auto t = drawTarget(); if (t) t->drawCircle(x, y, r, color); }
void WaveshareCanvas::fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) { auto t = drawTarget(); if (t) t->fillCircle(x, y, r, color); }
void WaveshareCanvas::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) { auto t = drawTarget(); if (t) t->drawLine(x0, y0, x1, y1, color); }

void WaveshareCanvas::drawString(const char* s, int32_t x, int32_t y) {
    auto t = drawTarget();
    if (!t || !s) return;
    t->setCursor(x, y);
    t->print(s);
}

void WaveshareCanvas::drawCentreString(const char* s, int32_t x, int32_t y) {
    int w = textWidth(s ? s : "");
    drawString(s ? s : "", x - (w / 2), y);
}

void WaveshareCanvas::drawRightString(const char* s, int32_t right, int32_t y) {
    int w = textWidth(s ? s : "");
    drawString(s ? s : "", right - w, y);
}

void WaveshareCanvas::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
    auto t = drawTarget();
    if (t) t->fillTriangle(x0, y0, x1, y1, x2, y2, color);
}

bool WaveshareCanvas::drawPng(File*, int32_t, int32_t) {
    return false;
}

uint16_t WaveshareCanvas::color565(uint8_t r, uint8_t g, uint8_t b) const {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

int WaveshareCanvas::textWidth(const char* s) const {
    if (!s) return 0;
#if defined(U8G2_FONT_SUPPORT)
    auto t = drawTarget();
    if (t) {
        int16_t x1 = 0;
        int16_t y1 = 0;
        uint16_t w = 0;
        uint16_t h = 0;
        t->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
        return static_cast<int>(w);
    }
#endif
    int len = 0;
    while (s[len] != '\0') len++;
    return len * (6 * textSize);
}

void WaveshareCanvas::pushSprite(int32_t, int32_t) {
    if (g_canvas) {
        g_canvas->flush(true);
    }
}

void WaveshareDeviceClass::DisplayClass::setRotation(uint8_t rotation) {
    if (g_gfx) g_gfx->setRotation(rotation);
}

void WaveshareDeviceClass::DisplayClass::setBrightness(uint8_t brightness) {
    if (g_sh8601) g_sh8601->setBrightness(brightness);
}

void WaveshareDeviceClass::SpeakerClass::begin() {
    ensureAudioReady();
}

void WaveshareDeviceClass::SpeakerClass::end() {
    if (!g_audioReady) return;
    g_audioI2s.end();
    g_audioReady = false;
}

void WaveshareDeviceClass::SpeakerClass::setVolume(uint8_t volume) {
    g_audioVolume = volume > 100 ? 100 : volume;
    if (g_es8311) {
        es8311_voice_volume_set(g_es8311, g_audioVolume, nullptr);
    }
}

void WaveshareDeviceClass::SpeakerClass::tone(uint16_t frequency, uint32_t durationMs, uint8_t) {
    if (frequency == 0 || durationMs == 0) return;
    if (!ensureAudioReady()) return;

    static constexpr uint32_t sampleRate = 16000;
    static constexpr size_t chunkSamples = 160;
    int16_t chunk[chunkSamples];
    uint32_t phase = 0;
    size_t remaining = (static_cast<size_t>(durationMs) * sampleRate) / 1000;
    while (remaining > 0) {
        size_t count = remaining > chunkSamples ? chunkSamples : remaining;
        for (size_t i = 0; i < count; ++i) {
            chunk[i] = (phase < (sampleRate / 2)) ? 6000 : -6000;
            phase += frequency;
            while (phase >= sampleRate) phase -= sampleRate;
        }
        playRaw(chunk, count, sampleRate, false, 1, 1, false);
        remaining -= count;
    }
}

void WaveshareDeviceClass::SpeakerClass::playRaw(int16_t* samples, size_t sampleCount, uint32_t sampleRate, bool stereo, uint8_t, uint8_t, bool) {
    if (!samples || sampleCount == 0) return;
    if (!ensureAudioReady()) return;
    if (sampleRate != 16000) {
        Serial.printf("[AUDIO] unsupported sample rate: %lu\n", static_cast<unsigned long>(sampleRate));
        return;
    }

    g_audioPlayingUntilMs = millis() + static_cast<uint32_t>((sampleCount * 1000ULL) / sampleRate) + 40;
    if (stereo) {
        g_audioI2s.write(reinterpret_cast<const uint8_t*>(samples), sampleCount * sizeof(int16_t));
    } else {
        static constexpr size_t monoChunk = 256;
        int16_t stereoChunk[monoChunk * 2];
        size_t offset = 0;
        while (offset < sampleCount) {
            size_t count = (sampleCount - offset) > monoChunk ? monoChunk : (sampleCount - offset);
            for (size_t i = 0; i < count; ++i) {
                int16_t sample = samples[offset + i];
                stereoChunk[i * 2] = sample;
                stereoChunk[i * 2 + 1] = sample;
            }
            g_audioI2s.write(reinterpret_cast<const uint8_t*>(stereoChunk), count * 2 * sizeof(int16_t));
            offset += count;
        }
    }
    g_audioPlayingUntilMs = millis() + 40;
}

bool WaveshareDeviceClass::SpeakerClass::isPlaying() const {
    return g_audioReady && millis() < g_audioPlayingUntilMs;
}

void WaveshareDeviceClass::SpeakerClass::stop() {
    g_audioPlayingUntilMs = 0;
}

void WaveshareDeviceClass::initDisplay() {
    if (g_gfx) return;
    bus = new Arduino_ESP32QSPI(
        LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3
    );
    gfx = new Arduino_SH8601(bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);
    g_sh8601 = gfx;
    g_gfx = gfx;
    displayReady = false;
    for (int i = 0; i < 5; ++i) {
        if (g_gfx->begin()) {
            displayReady = true;
            break;
        }
        delay(80);
    }
    if (displayReady) {
        g_sh8601->setBrightness(255);
        g_gfx->fillScreen(RGB565_WHITE);
        delay(120);
        g_gfx->fillScreen(RGB565_BLACK);
    }
}

void WaveshareDeviceClass::initTouch() {
    Wire.begin(IIC_SDA, IIC_SCL);
    bool expanderOk = expander.begin(0x20);
    if (!expanderOk) {
        touchReady = false;
        return;
    }
    expander.pinMode(0, OUTPUT);
    expander.pinMode(1, OUTPUT);
    expander.pinMode(2, OUTPUT);
    expander.digitalWrite(0, LOW);
    expander.digitalWrite(1, LOW);
    expander.digitalWrite(2, LOW);
    delay(20);
    expander.digitalWrite(0, HIGH);
    expander.digitalWrite(1, HIGH);
    expander.digitalWrite(2, HIGH);

    i2cBus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
    ft3168.reset(new Arduino_FT3x68(i2cBus, FT3168_DEVICE_ADDRESS, DRIVEBUS_DEFAULT_VALUE, TP_INT, touchInterruptHandler));
    g_touch = ft3168.get();
    touchReady = expanderOk;
    if (touchReady) {
        touchReady = ft3168->begin();
    }
    if (touchReady) {
        ft3168->IIC_Write_Device_State(
            ft3168->Arduino_IIC_Touch::Device::TOUCH_POWER_MODE,
            ft3168->Arduino_IIC_Touch::Device_Mode::TOUCH_POWER_MONITOR
        );
    }
}

void WaveshareDeviceClass::begin(const Config&) {
    initDisplay();
    inited = true;
}

bool WaveshareDeviceClass::sampleTouch(int32_t& x, int32_t& y, bool& pressed) {
    if (!inited || !touchReady || !ft3168) {
        pressed = false;
        x = lastTouchX;
        y = lastTouchY;
        return false;
    }
    int32_t fingers = (int32_t)ft3168->IIC_Read_Device_Value(
        ft3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
    bool nowPressed = (fingers > 0) || g_touchIrq;

    if (nowPressed) {
        g_touchIrq = false;
        int32_t tx = (int32_t)ft3168->IIC_Read_Device_Value(
            ft3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
        int32_t ty = (int32_t)ft3168->IIC_Read_Device_Value(
            ft3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
        if (tx >= 0 && ty >= 0) {
            if (tx >= LCD_WIDTH || ty >= LCD_HEIGHT) {
                if (tx < LCD_HEIGHT && ty < LCD_WIDTH) {
                    int32_t sx = tx;
                    tx = ty;
                    ty = sx;
                }
            }
            if (tx < 0) tx = 0;
            if (ty < 0) ty = 0;
            if (tx >= LCD_WIDTH) tx = LCD_WIDTH - 1;
            if (ty >= LCD_HEIGHT) ty = LCD_HEIGHT - 1;
            lastTouchX = tx;
            lastTouchY = ty;
        }
        lastTouchMs = millis();
    }
    x = lastTouchX;
    y = lastTouchY;
    pressed = nowPressed;
    return true;
}

void WaveshareDeviceClass::update() {
    if (!inited || !touchReady || !ft3168) return;
    Touch.detail.pressedEdge = false;
    Touch.detail.releasedEdge = false;

    int32_t tx = lastTouchX;
    int32_t ty = lastTouchY;
    bool nowPressed = false;
    sampleTouch(tx, ty, nowPressed);
    Touch.detail.x = tx;
    Touch.detail.y = ty;

    if (nowPressed && !touchPressed) {
        Touch.detail.pressedEdge = true;
    }
    if (!nowPressed && touchPressed) {
        Touch.detail.releasedEdge = true;
    }

    touchPressed = nowPressed;
    Touch.detail.pressed = touchPressed;
}

WaveshareDeviceClass M5;

#endif
