#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Wire.h>
#include <arduino-timer.h>
#include <driver/i2s.h>
#include "config.h"
#include "debug.h"
#include "DFRobotIRPosition.h"
#include "DFRobotIRPositionEx.h"
#include "GunJoyButton.h"
#include "GunFFB.h"
#include "GunHID.h"
#include "OpenFIRE_Diamond.h"
#include "OpenFIRE_Perspective.h"

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
    {PIN_BUTTON_B, INPUT_PULLUP},
    {PIN_TRIGGER, INPUT_PULLUP},

    {PIN_JOY_ADC_X, ANALOG},
    {PIN_JOY_ADC_Y, ANALOG},
    {PIN_BUTTON_START, INPUT_PULLUP},
    {PIN_BUTTON_SELECT, INPUT_PULLUP},
    {PIN_BUTTON_MODE, INPUT_PULLUP},
    {PIN_BUTTON_A, INPUT_PULLUP},
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
// static DFRobotIRPosition         _ir;
static DFRobotIRPositionEx      *_ir;
static ir_pos_t                  _pos[4];
static Adafruit_NeoPixel         _pixels(1, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);
static Timer<16, millis, void *> _timer;
static GunHID                   *_gunHID;
static GunJoyButton             *_gunJoy = new GunJoyButton();
static GunFFB                   *_gunFFB = new GunFFB();
static OpenFIRE_Diamond          _diamond;
static OpenFIRE_Perspective      _perspective;

static DFRobotIRPositionEx::Sensitivity_e _irSensitivity = DFRobotIRPositionEx::Sensitivity_Default;

/*
*****************************************************************************************
* FUNCTIONS
*****************************************************************************************
*/
int debug_printf(const char *format, ...) {
    Stream *stream;

    if (_gunHID) {
        stream = _gunHID->get_serial();
    } else {
        stream = &Serial0;
    }

    char    loc_buf[64];
    char   *temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if (len < 0) {
        va_end(arg);
        return 0;
    }
    if (len >= (int)sizeof(loc_buf)) {  // comparation of same sign type for the compiler
        temp = (char *)malloc(len + 1);
        if (temp == NULL) {
            va_end(arg);
            return 0;
        }
        len = vsnprintf(temp, len + 1, format, arg);
    }
    va_end(arg);
    return stream->write(temp, len);
}

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
        LOGV("%2d: %4d, %4d\n", i, pos[i].x, pos[i].y);
    }
    LOGV("------------------\n");
}

bool check_ir_camera(const char *m) {
    int error = _ir->basicAtomic(DFRobotIRPositionEx::Retry_2);
    if(error == DFRobotIRPositionEx::Error_Success) {
        _diamond.begin(_ir->xPositions(), _ir->yPositions(), _ir->seen());
        _perspective.warp(_diamond.X(0), _diamond.Y(0),
                            _diamond.X(1), _diamond.Y(1),
                            _diamond.X(2), _diamond.Y(2),
                            _diamond.X(3), _diamond.Y(3),
                            res_x / 2, 0, 0,
                            res_y / 2, res_x / 2,
                            res_y, res_x, res_y / 2);


        // const int *px = _ir->xPositions();
        // const int *py = _ir->yPositions();
        // const int flag = _ir->seen();

        // for (int i = 0; i < 4; i++) {
        //     _pos[i].x = *px++;
        //     _pos[i].y = *py++;
        // }
        // printResult(_pos, 4);
    }

    // _ir.requestPosition();
    // if (_ir.available()) {
    //     for (int i = 0; i < 4; i++) {
    //         _pos[i].x = _ir.readX(i);
    //         _pos[i].y = _ir.readY(i);
    //     }
    //     printResult(_pos, 4);
    // } else {
    //     LOGE("Device not available!\n");
    // }
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

void cb_button(GunJoyButton::btn_cb_info_t *cb) {
    static uint8_t buttons = 0;
    static int8_t x = 0;
    static int8_t y = 0;

    if (cb->mode == ANALOG) {
        if (cb->pin == PIN_JOY_ADC_X) {
            x = cb->val;
        } else if (cb->pin == PIN_JOY_ADC_Y) {
            y = cb->val;
        }
    } else {
        if (cb->val)
            buttons |= (1 << cb->no);
        else
            buttons &= ~(1 << cb->no);

        LOGV("no:%2d, pin:%2d, val:%4d, [x:%4d, y:%4d, buttons:%2x]\n", cb->no, cb->pin, cb->val, x, y, buttons);
    }
    _gunHID->report_gamepad(x, y, 0, buttons);

    switch (cb->pin) {
        case PIN_TRIGGER:
            if (_gunFFB && cb->val == 1)
                _gunFFB->trigger();
            break;
    }
}

void setup() {
    Serial0.begin(115200);
    LOGV("Start !!!\n");

    init_pins((pin_info_t *)_tbl_ctl_pins, ARRAY_SIZE(_tbl_ctl_pins));
    Wire.begin(PIN_IR_SDA, PIN_IR_SCL, 400000);
    ir_clk_init(0, 25000000, 48000);
    digitalWrite(PIN_IR_RESET, HIGH);


    _ir = new DFRobotIRPositionEx(Wire);
    // _ir->begin(400000, DFRobotIRPositionEx::DataFormat_Basic, _irSensitivity);
    // _timer.every(100, check_ir_camera, NULL);

    _pixels.begin();
    _pixels.show();
    _pixels.setBrightness(50);
    _pixels.setPixelColor(0, _pixels.Color(0, 0, 255));
    _pixels.show();

    //
    uint16_t auto_trg_delay = 300;
    uint16_t auto_trg_rpt_delay = 150;
    for (int i = 0; i < ARRAY_SIZE(_tbl_sw_pins); i++) {
        _gunJoy->add_button(i, _tbl_sw_pins[i].pin, _tbl_sw_pins[i].mode, cb_button);
    }
    _gunJoy->setup(auto_trg_delay, auto_trg_rpt_delay);

    // _gunHID = new GunHIDBLE("OpenFIRE", "FIRECon", 0xF143, 0x1998);
    _gunHID = new GunHIDUSB("OpenFIRE", "FIRECon", 0xF143, 0x1998);
    _gunHID->setup();
    _gunFFB->setup(PIN_SOLENOID, auto_trg_rpt_delay / 3, &_pixels, 0);
    // _gunFFB->setup(PIN_RUMBLE, auto_trg_rpt_delay / 3, &_pixels, 0);
}

void loop() {
    int ch;

    if (_gunHID->get_serial()->available()) {
        ch = _gunHID->get_serial()->read();

        switch (ch) {
            case '1':
                LOGV("auto trigger disabled\n");
                _gunJoy->set_auto_trigger(0, 0);
                break;

            case '2':
                LOGV("auto trigger enabled\n");
                _gunJoy->set_auto_trigger(300, 150);
                break;
        }
    }

    _gunHID->loop();
    _gunJoy->loop();
    _gunFFB->loop();
    _timer.tick();
}
