#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>
#include "DFRobotIRPosition.h"
#include "config.h"
#include "debug.h"

typedef struct {
    int x;
    int y;
} ir_pos_t;

static DFRobotIRPosition _ir;
static ir_pos_t  _pos[4];

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

void setup() {
    Serial.begin(115200);
    LOGV("Start !!!\n");

    pinMode(PIN_IR_RESET, OUTPUT);
    digitalWrite(PIN_IR_RESET, LOW);
    Wire.begin(PIN_IR_SDA, PIN_IR_SCL, 400000);
    ir_clk_init(0, 25000000, 48000);
    digitalWrite(PIN_IR_RESET, HIGH);
    _ir.begin();
}

void loop() {
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
    delay(100);
}
