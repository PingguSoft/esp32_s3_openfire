#include <Adafruit_NeoPixel.h>
#include <arduino-timer.h>

#include "GunCalibration.h"
#include "GunCamera.h"
#include "GunDock.h"
#include "GunFFB.h"
#include "GunHID.h"
#include "GunJoyButton.h"
#include "GunSettings.h"
#include "config.h"
#include "debug.h"

/*
*****************************************************************************************
* DATA TYPES
*****************************************************************************************
*/
typedef struct {
    uint8_t  pin;
    uint8_t  mode;
    uint16_t mouse_evt;
    uint16_t pad_evt;
    uint8_t  dock_evt;
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
    {PIN_TRIGGER, INPUT_PULLUP, MOUSE_LEFT, PAD_BUTTON_TR, 1},
    {PIN_BUTTON_A, INPUT_PULLUP, MOUSE_RIGHT, PAD_BUTTON_TL, 2},
    {PIN_BUTTON_START, INPUT_PULLUP, 0, PAD_BUTTON_START, 5},
    {PIN_BUTTON_SELECT, INPUT_PULLUP, 0, PAD_BUTTON_SELECT, 6},
    {PIN_BUTTON_C, INPUT_PULLUP, MOUSE_BACKWARD, PAD_BUTTON_A, 11},

    {PIN_JOY_ADC_Y, ANALOG, 0, 0, 0},
    {PIN_JOY_ADC_X, ANALOG, 0, 0, 0},
    {PIN_BUTTON_B, INPUT_PULLUP, MOUSE_MIDDLE, PAD_BUTTON_Y, 3},
};

/*
*****************************************************************************************
* VARIABLES
*****************************************************************************************
*/

/*
*****************************************************************************************
* FUNCTIONS
*****************************************************************************************
*/
int debug_printf(const char *format, ...) {
    Stream *stream;

    // if (_gunHID) {
    //     stream = _gunHID->get_serial();
    // } else {
    //     stream = &Serial0;
    // }
    stream = &Serial0;

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
    int ret = stream->write(temp, len);
    if (temp != loc_buf) {
        free(temp);
    }
    return ret;
}

class GunMain : public GunDockCallback {
   public:
    GunMain() {
        _gunJoy      = new GunJoyButton();
        _gunFFB      = new GunFFB();
        _gunCam      = new GunCamera();
        _gunCali     = new GunCalibration();
        _gunSettings = new GunSettings();
        _pixels      = new Adafruit_NeoPixel(1, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);
    }

    void onCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) {
        _gunSettings->onCallback(cmd, pData, size, stream);

        switch (cmd) {
            case GunDock::CMD_IR_BRIGHTNESS: {
                uint8_t lvl = *pData - '0';
                LOGV("ir brightness : %d\n", lvl);
                _gunCam->update_setting();
            } break;

            case GunDock::CMD_TEST_FFB:
                switch (*pData) {
                    case 's':  // solenoid
                    case 'r':  // rumble
                        update_force_feedback(*pData);
                        _gunFFB->trigger();
                        break;
                    case 'R':  // LED
                        _pixels->setPixelColor(0, _pixels->Color(255, 0, 0));
                        _pixels->show();
                        break;
                    case 'G':  // LED
                        _pixels->setPixelColor(0, _pixels->Color(0, 255, 0));
                        _pixels->show();
                        break;
                    case 'B':  // LED
                        _pixels->setPixelColor(0, _pixels->Color(0, 0, 255));
                        _pixels->show();
                        break;
                }
                break;

            case GunDock::CMD_REBOOT:
                esp_restart();
                break;

            case GunDock::CMD_SAVE_PREFERENCE:
                update_auto_trigger();
                update_force_feedback();
                break;

            default:
                break;
        }
    }

    void update_auto_trigger() {
        if (_gunSettings->get_gun_mode() == GunSettings::GunMode_Calibration ||
            !_gunSettings->get_feature_config()->autofireActive) {
            _gunJoy->set_auto_trigger(0, 0);
        } else if (_gunSettings->get_feature_config()->autofireActive) {
            GunSettings::params_map_t *params = _gunSettings->get_param_config();
            _gunJoy->set_auto_trigger(params->solenoidLongInterval, params->solenoidNormalInterval);
        }
    }

    void update_force_feedback(uint8_t mode = 0) {
        GunSettings::params_map_t *params = _gunSettings->get_param_config();

        if (mode == 's' || (mode == 0 && _gunSettings->get_feature_config()->solenoidActive)) {
            _gunFFB->setup(PIN_SOLENOID, params->solenoidFastInterval, _pixels, 0);
        } else if (mode == 'r' || (mode == 0 && _gunSettings->get_feature_config()->rumbleActive)) {
            _gunFFB->setup(PIN_RUMBLE, params->solenoidFastInterval, _pixels, 0);
        }
        _gunFFB->set_power(params->rumbleIntensity);
    }

    void setup() {
        bool is_cal_req;

        _prv_mode = GunSettings::GunMode_Init;

        // load settings
        _gunSettings->setup();
        is_cal_req = !_gunSettings->load();
        _gunSettings->set_gun_mode(is_cal_req ? GunSettings::GunMode_Calibration : GunSettings::GunMode_Run);

        // pixels
        _pixels->begin();
        _pixels->show();
        _pixels->setBrightness(50);
        _pixels->setPixelColor(0, _pixels->Color(0, 0, 255));
        _pixels->show();

        // joypad setting
        for (int i = 0; i < ARRAY_SIZE(_tbl_sw_pins); i++) {
            _gunJoy->add_button(_tbl_sw_pins[i].pin, _tbl_sw_pins[i].mode, _tbl_sw_pins[i].mouse_evt,
                                _tbl_sw_pins[i].pad_evt);
        }
        _gunJoy->setup();
        update_auto_trigger();
        update_force_feedback();

        // hid setup
        // _gunHID = new GunHIDBLE("OpenFIRE", "FIRECon", 0xF143, 0x1998);
        _gunHID = new GunHIDUSB("OpenFIRE", "FIRECon", 0xF143, _gunSettings->get_usb_config()->devicePID);
        _gunHID->setup();

        // camera setup
        _gunCam->setup(_gunSettings);

        // calibration setup
        _gunCali->setup(_gunSettings, _gunHID, _gunCam);
        if (is_cal_req) {
            LOGV("cali begin\n");
            _gunCali->begin();
        }

        // dock setup
        _gunDock = new GunDock(_gunHID->get_serial());
        _gunDock->set_callback(this);
    }

    void loop() {
        bool                   update;
        int8_t                 x, y;
        uint16_t               pad_buttons;
        uint8_t                mouse_buttons;
        GunSettings::GunMode_e mode;

        // process docking commands
        _gunDock->process();

        // input button processing
        update = _gunJoy->loop();
        _gunJoy->get(&x, &y, &pad_buttons, &mouse_buttons);

        // cam processing
        _gunCam->loop();

        // mode changed ?
        mode = _gunSettings->get_gun_mode();
        if (mode != _prv_mode) {
            _btn_trk.reset();
            _prv_mode = mode;
        }

        // mode specific works
        switch (mode) {
            case GunSettings::GunMode_Init:
                break;

            case GunSettings::GunMode_Calibration:
            case GunSettings::GunMode_Verification:
                if (!_gunCali->loop(pad_buttons)) {
                    _gunCali->end();
                    update_auto_trigger();
                }
                break;

            case GunSettings::GunMode_Docked:
                _btn_trk.begin(pad_buttons);
                for (int i = 0; i < ARRAY_SIZE(_tbl_sw_pins); i++) {
                    if (_btn_trk.isPressed(_tbl_sw_pins[i].pad_evt)) {
                        _gunDock->get_stream()->printf("Pressed: %d\r\n", _tbl_sw_pins[i].dock_evt);
                    } else if (_btn_trk.isReleased(_tbl_sw_pins[i].pad_evt)) {
                        _gunDock->get_stream()->printf("Released: %d\r\n", _tbl_sw_pins[i].dock_evt);
                    }
                }
                x = map(_gunCam->x(), 0, GunHID::mouse_max_x, -127, 127);
                y = map(_gunCam->y(), 0, GunHID::mouse_max_y, -127, 127);
                _gunHID->report_gamepad(x, y, 0, pad_buttons);
                proc_ir_test(_gunSettings, _gunCam);
                _btn_trk.end();
                break;

            case GunSettings::GunMode_Run:
                if (_gunCam->avail()) {
                    _gunHID->report_mouse(_gunCam->x(), _gunCam->y(), mouse_buttons);
                }
                x = map(_gunCam->x(), 0, GunHID::mouse_max_x, -127, 127);
                y = map(_gunCam->y(), 0, GunHID::mouse_max_y, -127, 127);
                _gunHID->report_gamepad(x, y, 0, pad_buttons);
                break;
        }

        // force feed back processing
        _gunFFB->loop();
    }

    void proc_ir_test(GunSettings *settings, GunCamera *cam) {
        GunSettings::profile_data_t *pd = settings->get_profile();
        if (pd->runMode != GunSettings::RunMode_Processing)
            return;

        // RAW Camera Output mapped to screen res (1920x1080)
        // RAW Output for viewing in processing sketch mapped to 1920x1080 screen resolution
        char  buf[120];
        char *pos = buf;

        for (int i = 0; i < 4; i++) {
            int rawX = map(cam->get_layout()->X(i), 0, 1023 << 2, 1920, 0);
            int rawY = map(cam->get_layout()->Y(i), 0, 768 << 2, 0, 1080);
            pos += sprintf(pos, "%d,%d,", rawX, rawY);
        }

        pos += sprintf(pos, "%d,%d,", cam->rx() >> 2, cam->ry() >> 2);

        // Median for viewing in processing
        int mx = map(cam->get_layout()->testMedianX(), 0, 1023 << 2, 1920, 0);
        int my = map(cam->get_layout()->testMedianY(), 0, 768 << 2, 0, 1080);
        pos += sprintf(pos, "%d,%d,\r\n", mx, my);
        _gunDock->get_stream()->print(buf);
    }

   private:
    Adafruit_NeoPixel     *_pixels;
    GunHID                *_gunHID;
    GunJoyButton          *_gunJoy;
    GunFFB                *_gunFFB;
    GunFFB                *_gunRumble;
    GunCamera             *_gunCam;
    GunCalibration        *_gunCali;
    GunSettings           *_gunSettings;
    GunDock               *_gunDock;
    ButtonTracker          _btn_trk;
    GunSettings::GunMode_e _prv_mode;
};

GunMain *_main = new GunMain();

void setup() {
    Serial0.begin(115200);
    LOGV("Start !!!\n");
    _main->setup();
}

void loop() {
    _main->loop();
}
