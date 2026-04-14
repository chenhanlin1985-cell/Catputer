#if defined(CATPUTER_WAVESHARE_DIAG)

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_XCA9554.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include <WiFi.h>

Adafruit_XCA9554 expander;
Arduino_DataBus* bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3
);
Arduino_SH8601* gfx = new Arduino_SH8601(
    bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT
);

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("[DIAG] boot");

    Wire.begin(IIC_SDA, IIC_SCL);
    bool ex = expander.begin(0x20);
    Serial.printf("[DIAG] expander=%d\n", ex ? 1 : 0);
    if (ex) {
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
    }

    bool ok = gfx->begin();
    Serial.printf("[DIAG] gfx->begin=%d\n", ok ? 1 : 0);
    if (ok) {
        gfx->setBrightness(255);
        gfx->fillScreen(RGB565_RED);
        delay(400);
        gfx->fillScreen(RGB565_GREEN);
        delay(400);
        gfx->fillScreen(RGB565_BLUE);
        delay(400);
        gfx->fillScreen(RGB565_BLACK);
        gfx->setTextColor(RGB565_WHITE);
        gfx->setCursor(20, 30);
        gfx->setTextSize(2);
        gfx->println("WAVESHARE DIAG OK");
    }
}

void loop() {
    static uint32_t last = 0;
    if (millis() - last > 1000) {
        last = millis();
        Serial.println("[DIAG] alive");
    }

    static uint32_t wifiLast = 0;
    if (millis() - wifiLast > 5000) {
        wifiLast = millis();
        bool modeOk = WiFi.mode(WIFI_STA);
        delay(20);
        wifi_mode_t mode = WiFi.getMode();
        Serial.printf("[DIAG][WIFI] mode_ok=%d mode=%d status=%d\n",
                      modeOk ? 1 : 0,
                      static_cast<int>(mode),
                      static_cast<int>(WiFi.status()));
    }
}

#endif
