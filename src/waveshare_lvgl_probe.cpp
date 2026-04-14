#if defined(CATPUTER_WAVESHARE_LVGL_PROBE)

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_XCA9554.h>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"
#include "lvgl.h"

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
static lv_obj_t *coordLabel = nullptr;
static lv_obj_t *counterLabel = nullptr;
static lv_obj_t *meter = nullptr;
static int pressCount = 0;

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
    int32_t touchX = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
    int32_t touchY = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
    if (FT3168->IIC_Interrupt_Flag) {
        FT3168->IIC_Interrupt_Flag = false;
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
        if (coordLabel) {
            lv_label_set_text_fmt(coordLabel, "x:%d y:%d", static_cast<int>(touchX), static_cast<int>(touchY));
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

static void lvTick(void *) {
    lv_tick_inc(2);
}

static void onButton(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    pressCount++;
    if (counterLabel) {
        lv_label_set_text_fmt(counterLabel, "press:%d", pressCount);
    }
    if (meter) {
        lv_arc_set_value(meter, (pressCount * 7) % 100);
    }
}

static void buildUi() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x081018), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "Waveshare LVGL Probe");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    coordLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(coordLabel, "x:- y:-");
    lv_obj_align(coordLabel, LV_ALIGN_TOP_LEFT, 16, 40);

    counterLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(counterLabel, "press:0");
    lv_obj_align(counterLabel, LV_ALIGN_TOP_RIGHT, -16, 40);

    meter = lv_arc_create(lv_scr_act());
    lv_obj_set_size(meter, 180, 180);
    lv_obj_align(meter, LV_ALIGN_CENTER, 0, -20);
    lv_arc_set_rotation(meter, 135);
    lv_arc_set_bg_angles(meter, 0, 270);
    lv_arc_set_range(meter, 0, 100);
    lv_arc_set_value(meter, 20);

    lv_obj_t *button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(button, 220, 64);
    lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, -28);
    lv_obj_add_event_cb(button, onButton, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *buttonText = lv_label_create(button);
    lv_label_set_text(buttonText, "Touch Me");
    lv_obj_center(buttonText);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(IIC_SDA, IIC_SCL);
    bool expanderOk = expander.begin(0x20);
    if (!expanderOk) {
        while (true) {
            delay(1000);
        }
    }
    expander.pinMode(0, OUTPUT);
    expander.pinMode(1, OUTPUT);
    expander.pinMode(2, OUTPUT);
    expander.pinMode(6, OUTPUT);
    expander.digitalWrite(0, LOW);
    expander.digitalWrite(1, LOW);
    expander.digitalWrite(2, LOW);
    expander.digitalWrite(6, LOW);
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

    lv_init();
    buf1 = static_cast<lv_color_t *>(heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT / 4 * sizeof(lv_color_t), MALLOC_CAP_DMA));
    buf2 = static_cast<lv_color_t *>(heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT / 4 * sizeof(lv_color_t), MALLOC_CAP_DMA));
    lv_disp_draw_buf_init(&drawBuf, buf1, buf2, LCD_WIDTH * LCD_HEIGHT / 4);

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
}

void loop() {
    lv_timer_handler();
    delay(5);
}

#endif
