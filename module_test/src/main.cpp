#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <arduino-timer.h>
#include <Adafruit_NeoPixel.h>
#include "DFRobotIRPosition.h"
#include "config.h"
#include "debug.h"

typedef struct {
    int x;
    int y;
} ir_pos_t;

static DFRobotIRPosition _ir;
static ir_pos_t  _pos[4];

static Adafruit_NeoPixel _pixels(1, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);
static Timer<16, millis, const char *> _timer;

void ir_clk_init(int port, int mclk, uint32_t hz) {
    i2s_config_t i2s_config_dac = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = hz,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,    // lowest interrupt priority
            .dma_buf_count = 2,
            .dma_buf_len = 128,
            .use_apll = true,
            .tx_desc_auto_clear = true,    // Silence on underflow
            .fixed_mclk = mclk,
            .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,    // Unused
            .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT     // Use bits per sample
    };
    if (i2s_driver_install((i2s_port_t)port, &i2s_config_dac, 0, NULL) != ESP_OK)
        LOGE("error i2s driver install...\n");

    i2s_pin_config_t pins = {.mck_io_num = PIN_IR_CLK, .bck_io_num = I2S_PIN_NO_CHANGE, .ws_io_num = I2S_PIN_NO_CHANGE, .data_out_num = I2S_PIN_NO_CHANGE, .data_in_num = I2S_PIN_NO_CHANGE};
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

typedef struct {
    uint8_t pin;
    uint8_t mode;
    uint8_t value;
} pin_mode_t;

const pin_mode_t _tbl_pins[] = {
    { PIN_IR_RESET,  OUTPUT, LOW },

    { PIN_RUMBLE, OUTPUT, LOW },
    { PIN_RECOIL, OUTPUT, LOW },
    { PIN_BUTTON_B, INPUT_PULLUP, HIGH },
    { PIN_TRIGGER,  INPUT_PULLUP, HIGH },
};


bool print_message(const char *m) {
  Serial.print("print_message: ");
  Serial.println(m);
  return true; // repeat? true
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

void setup() {
    Serial.begin(115200);
    LOGV("Start !!!\n");

    for (int i = 0; i < ARRAY_SIZE(_tbl_pins); i++) {
        pinMode(_tbl_pins[i].pin, _tbl_pins[i].mode);
        if (_tbl_pins[i].mode == OUTPUT) {
            digitalWrite(_tbl_pins[i].pin, _tbl_pins[i].value);
        }
    }

    Wire.begin(PIN_IR_SDA, PIN_IR_SCL, 400000);
    ir_clk_init(0, 25000000, 48000);
    digitalWrite(PIN_IR_RESET, HIGH);
    _ir.begin();

    _pixels.begin();
    _pixels.show();
    _pixels.setBrightness(50);
    _pixels.setPixelColor(0, _pixels.Color(  0,   0, 255));
    _pixels.show();

    _timer.every(100, check_ir_camera, NULL);
}

bool _is_trigger = false;
void *_timer_task;
char *_state;

bool off_motors(const char *param) {
    // digitalWrite(PIN_RECOIL, LOW);
    analogWrite(PIN_RUMBLE, 0);

    _pixels.setPixelColor(0, _pixels.Color(0, 0, 255));
    _pixels.show();

    return false;
}

bool check_trigger(const char *state) {
    int8_t trigger = digitalRead(PIN_TRIGGER);

    if (trigger == LOW) {
        LOGV("BANG !!!\n");
        // digitalWrite(PIN_RECOIL, HIGH);

        analogWrite(PIN_RUMBLE, 255);
        _pixels.setPixelColor(0, _pixels.Color(255, 0, 0));
        _pixels.show();
        _timer.in(50, off_motors, NULL);

        if (state == 0) {
            _state = (char*)1;
            _timer.in(300, check_trigger, _state);
        } else if (state == (char*)1) {
            _state = (char*)2;
            _timer.every(150, check_trigger, _state);
        }
    } else {
        _is_trigger = false;
    }
    return ((_state == (char*)2) && trigger == LOW);
}

void loop() {
    int8_t trigger = digitalRead(PIN_TRIGGER);

    if (trigger == LOW && !_is_trigger) {
        _is_trigger = true;
        _state = 0; // debounce
        _timer.in(10, check_trigger, _state);
    }
    _timer.tick();
}
