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

bool GunCamera::timer_camera_task(GunCamera *p) {
    int error = p->_ir->basicAtomic(DFRobotIRPositionEx::Retry_2);

    if (error == DFRobotIRPositionEx::Error_Success) {
        if (p->_settings->get_gun_mode() == GunSettings::GunMode_Calibration) {
            if (p->_ir->seen() != p->_last_seen) {
                LOGV("IR detected %d %X\n", p->_ir->avail(), p->_ir->seen());
            }
        }

        p->_layout->begin(p->_ir->xPositions(), p->_ir->yPositions(), p->_ir->seen());
        p->_perspective->warp(p->_layout->X(0), p->_layout->Y(0), p->_layout->X(1), p->_layout->Y(1), p->_layout->X(2),
                              p->_layout->Y(2), p->_layout->X(3), p->_layout->Y(3), res_x / 2, 0, 0, res_y / 2,
                              res_x / 2, res_y, res_x, res_y / 2);

        // Output mapped to screen resolution because offsets are measured in pixels
        GunSettings::profile_data_t *pd = p->_settings->get_profile_data();
        int x = map(p->_perspective->getX(), 0, res_x, (0 - pd->leftOffset), (res_x + pd->rightOffset));
        int y = map(p->_perspective->getY(), 0, res_y, (0 - pd->topOffset), (res_y + pd->bottomOffset));

        switch (pd->runMode) {
            case GunSettings::RunMode_Average:
                // 2 position moving average
                p->_idx ^= 1;
                p->_xAxisArr[p->_idx] = x;
                p->_yAxisArr[p->_idx] = y;
                x                 = (p->_xAxisArr[0] + p->_xAxisArr[1]) / 2;
                y                 = (p->_yAxisArr[0] + p->_yAxisArr[1]) / 2;
                break;

            case GunSettings::RunMode_Average2:
                // weighted average of current position and previous 2
                p->_idx = (p->_idx < 2) ? (p->_idx + 1) : 0;
                p->_xAxisArr[p->_idx] = x;
                p->_yAxisArr[p->_idx] = y;
                x                 = (x + p->_xAxisArr[0] + p->_xAxisArr[1] + p->_xAxisArr[2]) / 4;
                y                 = (y + p->_yAxisArr[0] + p->_yAxisArr[1] + p->_yAxisArr[2]) / 4;
                break;

            default:
                break;
        }

        p->_rx = x;
        p->_ry = y;

        // Constrain that bisch so negatives don't cause underflow
        int cx = constrain(x, 0, res_x);
        int cy = constrain(y, 0, res_y);

        // Output mapped to Mouse resolution
        p->_x = map(cx, 0, res_x, 0, 32767);
        p->_y = map(cy, 0, res_y, 0, 32767);

        p->update_last_seen();

#if 0
        GunPreferences::GunMode_e gunMode = p->_settings->get_gun_mode();
        if (gunMode == GunPreferences::GunMode_Run) {
            p->update_last_seen();

            // if (serialARcorrection) {
            //     p->_x = map(p->_x, 4147, 28697, 0, 32767);
            //     p->_x = constrain(p->_x, 0, 32767);
            // }

            bool offXAxis = false;
            bool offYAxis = false;

            if (p->_x == 0 || p->_x == 32767) {
                offXAxis = true;
            }

            if (p->_y == 0 || p->_y == 32767) {
                offYAxis = true;
            }

            // if (offXAxis || offYAxis) {
            //     buttons.offScreen = true;
            // } else {
            //     buttons.offScreen = false;
            // }

            // if (buttons.analogOutput) {
            //     Gamepad16.moveCam(p->_x, p->_y);
            // } else {
            //     AbsMouse5.move(p->_x, p->_y);
            // }
        } else {
            // RAW Camera Output mapped to screen res (1920x1080)
            int rawX[4];
            int rawY[4];

            // RAW Output for viewing in processing sketch mapped to 1920x1080 screen resolution
            if (pd->irLayout) {
                for (int i = 0; i < 4; i++) {
                    rawX[i] = map(p->_layout->X(i), 0, 1023 << 2, 1920, 0);
                    rawY[i] = map(p->_layout->Y(i), 0, 768 << 2, 0, 1080);
                }
            }
            if (pd->runMode == GunPreferences::RunMode_Processing) {
                int mx, my;

                if (pd->irLayout) {
                    // Median for viewing in processing
                    mx = map(p->_layout->testMedianX(), 0, 1023 << 2, 1920, 0);
                    my = map(p->_layout->testMedianY(), 0, 768 << 2, 0, 1080);
                }
                // LOGV("(%4d, %4d), (%4d, %4d), (%4d, %4d), (%4d, %4d) => (%4d, %4d), (%4d, %4d)\n", rawX[0], rawY[0],
                //      rawX[1], rawY[1], rawX[2], rawY[2], rawX[3], rawY[3], x >> 2, y >> 2, mx, my);
            }
#ifdef USES_DISPLAY
            OLED.DrawVisibleIR(rawX, rawY);
#endif  // USES_DISPLAY
        }
#endif
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

    GunSettings::profile_data_t *pd = _settings->get_profile_data();
    _layout = (pd->irLayout) ? (OpenFIRE_Layout *)new OpenFIRE_Diamond() : (OpenFIRE_Layout *)new OpenFIRE_Square();
    _perspective->source(pd->adjX, pd->adjY);
    _perspective->deinit(0);

    _ir->begin(400000, DFRobotIRPositionEx::DataFormat_Basic, _irSensitivity);
    _timer->every(10, timer_camera_task, this);
}

void GunCamera::loop() {
    _timer->tick();
}
