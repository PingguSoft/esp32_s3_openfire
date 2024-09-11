#include "GunCamera.h"

#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>

#include "OpenFIRE_Diamond.h"
#include "OpenFIRE_Square.h"

/*
*****************************************************************************************
* DATA TYPES
*****************************************************************************************
*/
void GunCamera::clk_init(int port, int mclk, uint32_t hz) {
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

void GunCamera::update_last_seen() {
    _last_seen = _layout->seen();
}

void printResult(const int x[], const int y[], const int seen) {
    LOGV("%4x : (%4d, %4d), (%4d, %6d), (%4d, %4d), (%4d, %4d)\n", seen, x[0], y[0], x[1], y[1], x[2], y[2], x[3], y[3]);
}

bool GunCamera::timer_camera_task(GunCamera *cam) {
    int error = cam->_ir->basicAtomic(DFRobotIRPositionEx::Retry_2);

    if (error == DFRobotIRPositionEx::Error_Success) {
        if (cam->_settings->get_gun_mode() == GunSettings::GunMode_Calibration) {
            if (cam->_ir->seen() != cam->_last_seen) {
                LOGV("IR detected %d %X\n", cam->_ir->avail(), cam->_ir->seen());
            }
        }

        cam->_layout->begin(cam->_ir->xPositions(), cam->_ir->yPositions(), cam->_ir->seen());
        cam->_perspective->warp(cam->_layout->X(0), cam->_layout->Y(0), cam->_layout->X(1), cam->_layout->Y(1), cam->_layout->X(2),
                              cam->_layout->Y(2), cam->_layout->X(3), cam->_layout->Y(3), res_x / 2, 0, 0, res_y / 2,
                              res_x / 2, res_y, res_x, res_y / 2);

        // Output mapped to screen resolution because offsets are measured in pixels
        GunSettings::profile_data_t *pd = cam->_settings->get_profile();
        int x = map(cam->_perspective->getX(), 0, res_x, (0 - pd->leftOffset), (res_x + pd->rightOffset));
        int y = map(cam->_perspective->getY(), 0, res_y, (0 - pd->topOffset), (res_y + pd->bottomOffset));

        switch (pd->runMode) {
            case GunSettings::RunMode_Average:
                // 2 position moving average
                cam->_idx ^= 1;
                cam->_xAxisArr[cam->_idx] = x;
                cam->_yAxisArr[cam->_idx] = y;
                x                 = (cam->_xAxisArr[0] + cam->_xAxisArr[1]) / 2;
                y                 = (cam->_yAxisArr[0] + cam->_yAxisArr[1]) / 2;
                break;

            case GunSettings::RunMode_Average2:
                // weighted average of current position and previous 2
                cam->_idx = (cam->_idx < 2) ? (cam->_idx + 1) : 0;
                cam->_xAxisArr[cam->_idx] = x;
                cam->_yAxisArr[cam->_idx] = y;
                x                 = (x + cam->_xAxisArr[0] + cam->_xAxisArr[1] + cam->_xAxisArr[2]) / 4;
                y                 = (y + cam->_yAxisArr[0] + cam->_yAxisArr[1] + cam->_yAxisArr[2]) / 4;
                break;

            default:
                break;
        }

        cam->_rx = x;
        cam->_ry = y;

        // Constrain that bisch so negatives don't cause underflow
        int cx = constrain(x, 0, res_x);
        int cy = constrain(y, 0, res_y);

        // Output mapped to Mouse resolution
        cam->_x = map(cx, 0, res_x, 0, 32767);
        cam->_y = map(cy, 0, res_y, 0, 32767);
        cam->update_last_seen();
    } else if (error != DFRobotIRPositionEx::Error_DataMismatch) {
        LOGE("Device not available!\n");
    }

    return true;
}

void GunCamera::setup(GunSettings *settings) {
    pinMode(PIN_IR_RESET, OUTPUT);
    digitalWrite(PIN_IR_RESET, LOW);
    clk_init(0, 25000000, 48000);
    Wire.begin(PIN_IR_SDA, PIN_IR_SCL, 400000);
    digitalWrite(PIN_IR_RESET, HIGH);

    _timer       = new Timer<1, millis, GunCamera *>();
    _ir          = new DFRobotIRPositionEx(Wire);
    _perspective = new OpenFIRE_Perspective();
    _settings = settings;

    GunSettings::profile_data_t *pd = _settings->get_profile();
    _ir->begin(400000, DFRobotIRPositionEx::DataFormat_Basic, (DFRobotIRPositionEx::Sensitivity_e)pd->irSensitivity);
    update_setting();
    _timer->every(10, timer_camera_task, this);
}

void GunCamera::update_setting() {
    GunSettings::profile_data_t *pd = _settings->get_profile();
    if (_layout)
        delete _layout;

    _layout = (pd->irLayout) ? (OpenFIRE_Layout *)new OpenFIRE_Diamond() : (OpenFIRE_Layout *)new OpenFIRE_Square();
    _perspective->source(pd->adjX, pd->adjY);
    _perspective->deinit(0);
    _ir->sensitivityLevel((DFRobotIRPositionEx::Sensitivity_e)pd->irSensitivity);
}

void GunCamera::loop() {
    _timer->tick();
}
