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
    {PIN_TRIGGER, INPUT_PULLUP},      {PIN_BUTTON_A, INPUT_PULLUP},      {PIN_BUTTON_B, INPUT_PULLUP},
    {PIN_BUTTON_START, INPUT_PULLUP}, {PIN_BUTTON_SELECT, INPUT_PULLUP},

    {PIN_JOY_ADC_Y, ANALOG},          {PIN_JOY_ADC_X, ANALOG},           {PIN_BUTTON_MODE, INPUT_PULLUP},
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
            }
            break;

            case GunDock::CMD_TEST_FFB:
                switch (*pData) {
                    case 's':   // solenoid
                        break;
                    case 'r':   // rumble
                        break;
                    case 'R':   // LED
                        break;
                    case 'G':   // LED
                        break;
                    case 'B':   // LED
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

    void process_joy(int8_t x, int8_t y, uint8_t buttons) {
        static ButtonTracker btn_trk;

        btn_trk.begin(buttons);

        if (btn_trk.isToggled())
            LOGV("x:%4d, y:%4d, buttons:%2x\n", x, y, buttons);

        _gunHID->report_gamepad(x, y, 0, buttons);

        if (btn_trk.isPressed(GunSettings::BtnMask_Trigger)) {
            if (_gunFFB)
                _gunFFB->trigger();
            _gunHID->report_mouse(300, 300, 0);
        } else if (btn_trk.isReleased(GunSettings::BtnMask_Trigger)) {
            _gunHID->report_mouse(32768 / 2, 32768 / 2, 0);
        }

        btn_trk.end();
    }

    void update_auto_trigger() {
        if (_gunSettings->get_gun_mode() == GunSettings::GunMode_Calibration || !_gunSettings->get_feature_config()->autofireActive) {
            _gunJoy->set_auto_trigger(0, 0);
        } else if (_gunSettings->get_feature_config()->autofireActive) {
            GunSettings::params_map_t *params = _gunSettings->get_param_config();
            _gunJoy->set_auto_trigger(params->solenoidLongInterval, params->solenoidNormalInterval);
        }
    }

    void update_force_feedback() {
        GunSettings::params_map_t *params = _gunSettings->get_param_config();

        if (_gunSettings->get_feature_config()->solenoidActive) {
            _gunFFB->setup(PIN_SOLENOID, params->solenoidFastInterval, _pixels, 0);
        } else if (_gunSettings->get_feature_config()->rumbleActive) {
            _gunFFB->setup(PIN_RUMBLE, params->solenoidFastInterval, _pixels, 0);
        }
        _gunFFB->set_power(params->rumbleIntensity);
    }

    void setup() {
        bool is_cal_req;

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
            _gunJoy->add_button(i, _tbl_sw_pins[i].pin, _tbl_sw_pins[i].mode);
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

        _gunDock = new GunDock(_gunHID->get_serial());
        _gunDock->set_callback(this);
    }

    void loop() {
        int     ch;
        int8_t  x, y;
        uint8_t buttons;

        /*
            if (_gunHID->get_serial()->available()) {
                ch = _gunHID->get_serial()->read();
                debug_printf("%02X ", ch);

                switch (ch) {
                    case '1':
                        LOGV("auto trigger disabled\n");
                        _gunJoy->set_auto_trigger(0, 0);
                        break;

                    case '2':
                        LOGV("auto trigger enabled\n");
                        _gunJoy->set_auto_trigger(300, 150);
                        break;

                    case '0':
                        _gunSettings->load();
                        break;

                    case '[':
                        LOGV("cali begin\n");
                        _gunCali->begin();
                        break;

                    case ']':
                        LOGV("cali end\n");
                        _gunCali->end();
                        break;
                }
            }
        */

        _gunDock->process();

#if 0
        bool update = _gunJoy->loop();
        _gunJoy->get(&x, &y, &buttons);
        _gunCam->loop();

        GunSettings::GunMode_e mode = _gunSettings->get_gun_mode();
        switch (mode) {
            case GunSettings::GunMode_Init:
                break;

            case GunSettings::GunMode_Calibration:
            case GunSettings::GunMode_Verification:
                if (!_gunCali->loop(buttons)) {
                    _gunSettings->set_gun_mode(GunSettings::GunMode_Run);
                    update_auto_trigger();
                }
                break;

            case GunSettings::GunMode_Run:
                if (_gunCam->avail())
                    _gunHID->report_mouse(_gunCam->x(), _gunCam->y(), 0);
                break;
        }

        _gunFFB->loop();
#endif
    }



   private:
    Adafruit_NeoPixel *_pixels;
    GunHID            *_gunHID;
    GunJoyButton      *_gunJoy;
    GunFFB            *_gunFFB;
    GunCamera         *_gunCam;
    GunCalibration    *_gunCali;
    GunSettings       *_gunSettings;
    GunDock           *_gunDock;
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
