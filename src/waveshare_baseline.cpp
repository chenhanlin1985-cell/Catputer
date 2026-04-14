#if defined(CATPUTER_WAVESHARE_BASELINE)

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_XCA9554.h>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"

static Adafruit_XCA9554 expander;
static std::shared_ptr<Arduino_IIC_DriveBus> i2cBus;
static std::unique_ptr<Arduino_IIC> ft3168;
static volatile bool touchIrq = false;

static Arduino_DataBus* bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3
);
static Arduino_SH8601* gfx = new Arduino_SH8601(
    bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT
);

static int lastX = -1;
static int lastY = -1;
static bool lastPressed = false;

static void onTouchInterrupt() {
    touchIrq = true;
    if (ft3168) {
        ft3168->IIC_Interrupt_Flag = true;
    }
}

static void drawStaticUi() {
    gfx->fillScreen(RGB565_BLACK);
    gfx->setBrightness(220);
    gfx->fillRect(0, 0, LCD_WIDTH, 36, RGB565_BLUE);
    gfx->setTextColor(RGB565_WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(12, 10);
    gfx->print("BASELINE");
    gfx->drawRect(12, 56, LCD_WIDTH - 24, LCD_HEIGHT - 120, RGB565_WHITE);
    gfx->setCursor(20, LCD_HEIGHT - 48);
    gfx->print("Touch test area");
}

static void drawTouchPoint(int x, int y, bool pressed) {
    if (lastPressed && lastX >= 0 && lastY >= 0) {
        gfx->fillCircle(lastX, lastY, 10, RGB565_BLACK);
    }
    gfx->fillRect(0, 40, LCD_WIDTH, 14, RGB565_BLACK);
    gfx->setCursor(8, 40);
    gfx->setTextColor(pressed ? RGB565_GREEN : RGB565_RED);
    char buf[64];
    snprintf(buf, sizeof(buf), "pressed=%d x=%d y=%d", pressed ? 1 : 0, x, y);
    gfx->print(buf);
    if (pressed) {
        gfx->fillCircle(x, y, 10, RGB565_GREEN);
    }
    lastX = x;
    lastY = y;
    lastPressed = pressed;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("[BASELINE] boot");

    Wire.begin(IIC_SDA, IIC_SCL);
    bool ex = expander.begin(0x20);
    Serial.printf("[BASELINE] expander=%d\n", ex ? 1 : 0);
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
    Serial.printf("[BASELINE] gfx=%d\n", ok ? 1 : 0);
    if (!ok) {
        return;
    }
    drawStaticUi();

    i2cBus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
    ft3168.reset(new Arduino_FT3x68(i2cBus, FT3168_DEVICE_ADDRESS, DRIVEBUS_DEFAULT_VALUE, TP_INT, onTouchInterrupt));
    bool tOk = ft3168->begin();
    Serial.printf("[BASELINE] touch=%d\n", tOk ? 1 : 0);
    if (tOk) {
        ft3168->IIC_Write_Device_State(
            ft3168->Arduino_IIC_Touch::Device::TOUCH_POWER_MODE,
            ft3168->Arduino_IIC_Touch::Device_Mode::TOUCH_POWER_MONITOR
        );
    }
}

void loop() {
    if (!ft3168) {
        delay(200);
        return;
    }

    int fingers = (int)ft3168->IIC_Read_Device_Value(
        ft3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
    bool pressed = fingers > 0 || touchIrq;
    int x = lastX < 0 ? LCD_WIDTH / 2 : lastX;
    int y = lastY < 0 ? LCD_HEIGHT / 2 : lastY;

    if (pressed) {
        touchIrq = false;
        int tx = (int)ft3168->IIC_Read_Device_Value(
            ft3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
        int ty = (int)ft3168->IIC_Read_Device_Value(
            ft3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
        if (tx >= 0 && ty >= 0) {
            if (tx >= LCD_WIDTH || ty >= LCD_HEIGHT) {
                if (tx < LCD_HEIGHT && ty < LCD_WIDTH) {
                    int sx = tx;
                    tx = ty;
                    ty = sx;
                }
            }
            if (tx < 0) tx = 0;
            if (ty < 0) ty = 0;
            if (tx >= LCD_WIDTH) tx = LCD_WIDTH - 1;
            if (ty >= LCD_HEIGHT) ty = LCD_HEIGHT - 1;
            x = tx;
            y = ty;
        }
    }

    if (pressed != lastPressed || x != lastX || y != lastY) {
        drawTouchPoint(x, y, pressed);
        Serial.printf("[BASELINE] pressed=%d x=%d y=%d\n", pressed ? 1 : 0, x, y);
    }
    delay(20);
}

#endif
