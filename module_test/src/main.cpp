#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Wire.h>
#include <arduino-timer.h>
#include <driver/i2s.h>

#include "DFRobotIRPosition.h"
#include "GunButton.h"
#include "GunFFB.h"
#include "GunHID.h"
#include "config.h"
#include "debug.h"

/*
*****************************************************************************************
* DATA TYPES
*****************************************************************************************
*/
typedef struct {
    int x;
    int y;
} ir_pos_t;

typedef struct {
    uint8_t pin;
    uint8_t mode;
} pin_info_t;

/*
*****************************************************************************************
* FUNCTION PREDEFINITION
*****************************************************************************************
*/

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/
static const pin_info_t _tbl_sw_pins[] = {
    {PIN_BUTTON_B, INPUT_PULLUP},     {PIN_TRIGGER, INPUT_PULLUP},

    {PIN_JOY_ADC_X, INPUT_PULLUP},    {PIN_JOY_ADC_Y, INPUT_PULLUP},
    {PIN_BUTTON_START, INPUT_PULLUP}, {PIN_BUTTON_SELECT, INPUT_PULLUP},
    {PIN_BUTTON_MODE, INPUT_PULLUP},  {PIN_BUTTON_A, INPUT_PULLUP},
};

static const pin_info_t _tbl_ctl_pins[] = {
    {PIN_IR_RESET, OUTPUT},

    {PIN_RUMBLE, OUTPUT},
    {PIN_SOLENOID, OUTPUT},
};

/*
*****************************************************************************************
* VARIABLES
*****************************************************************************************
*/
static DFRobotIRPosition         _ir;
static ir_pos_t                  _pos[4];
static Adafruit_NeoPixel         _pixels(1, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);
static Timer<16, millis, void *> _timer;
static GunHID                   *_gunhid;
static GunButton                *_gunBtn = new GunButton();
static GunFFB                   *_gunFFB = new GunFFB();

/*
*****************************************************************************************
* FUNCTIONS
*****************************************************************************************
*/
void ir_clk_init(int port, int mclk, uint32_t hz) {
    i2s_config_t i2s_config_dac = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = hz,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,  // lowest interrupt priority
        .dma_buf_count        = 2,
        .dma_buf_len          = 128,
        .use_apll             = true,
        .tx_desc_auto_clear   = true,  // Silence on underflow
        .fixed_mclk           = mclk,
        .mclk_multiple        = I2S_MCLK_MULTIPLE_DEFAULT,  // Unused
        .bits_per_chan        = I2S_BITS_PER_CHAN_DEFAULT   // Use bits per sample
    };
    if (i2s_driver_install((i2s_port_t)port, &i2s_config_dac, 0, NULL) != ESP_OK)
        LOGE("error i2s driver install...\n");

    i2s_pin_config_t pins = {.mck_io_num   = PIN_IR_CLK,
                             .bck_io_num   = I2S_PIN_NO_CHANGE,
                             .ws_io_num    = I2S_PIN_NO_CHANGE,
                             .data_out_num = I2S_PIN_NO_CHANGE,
                             .data_in_num  = I2S_PIN_NO_CHANGE};
    i2s_set_pin((i2s_port_t)port, &pins);
    i2s_set_sample_rates((i2s_port_t)port, hz);
}

void printResult(ir_pos_t *pos, int size) {
    int cnt = 0;

    for (int i = 0; i < size; i++) {
        if (pos[i].x != 1023 && pos[i].y != 1023) {
            LOGV("%2d: %4d, %4d\n", i, pos[i].x, pos[i].y);
            cnt++;
        }
    }
    if (cnt > 0)
        LOGV("------------------\n");
}

bool check_ir_camera(const char *m) {
    _ir.requestPosition();
    if (_ir.available()) {
        for (int i = 0; i < 4; i++) {
            _pos[i].x = _ir.readX(i);
            _pos[i].y = _ir.readY(i);
        }
        printResult(_pos, 4);
    } else {
        LOGE("Device not available!\n");
    }
    return true;
}

void init_pins(pin_info_t *tbl, int8_t len) {
    for (int i = 0; i < len; i++) {
        pinMode(tbl->pin, tbl->mode);
        if (tbl->mode == OUTPUT) {
            digitalWrite(tbl->pin, LOW);
        }
        tbl++;
    }
}

void cb_button(void *param) {
    pin_info_t *info = (pin_info_t*)param;

    LOGV("pin:%d\n", info->pin);

    switch (info->pin) {
        case PIN_TRIGGER:
            if (_gunFFB)
                _gunFFB->trigger();
            break;
    }
}

void setup() {
    Serial.begin(115200);
    LOGV("Start !!!\n");

    init_pins((pin_info_t *)_tbl_ctl_pins, ARRAY_SIZE(_tbl_ctl_pins));
    Wire.begin(PIN_IR_SDA, PIN_IR_SCL, 400000);
    ir_clk_init(0, 25000000, 48000);
    digitalWrite(PIN_IR_RESET, HIGH);
    // _ir.begin();
    // _timer.every(100, check_ir_camera, NULL);

    _pixels.begin();
    _pixels.show();
    _pixels.setBrightness(50);
    _pixels.setPixelColor(0, _pixels.Color(0, 0, 255));
    _pixels.show();

    //
    //
    //
    for (int i = 0; i < ARRAY_SIZE(_tbl_sw_pins); i++) {
        _gunBtn->add_button(_tbl_sw_pins[i].pin, _tbl_sw_pins[i].mode, cb_button);
    }
    _gunBtn->setup();

    _gunhid = new GunHIDUSB("OpenFIRE", "FIRECon", 0xF143, 0x1998);
    // _gunhid = new GunHIDBLE("OpenFIRE", "FIRECon", 0xF143, 0x1998);
    _gunhid->setup();
    _gunFFB->setup(PIN_SOLENOID, &_pixels, 0);
}

void loop() {
    _gunhid->loop();
    _gunBtn->loop();
    _gunFFB->loop();
    _timer.tick();
}
