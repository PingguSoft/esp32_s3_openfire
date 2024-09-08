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

#define OPENFIRE_VERSION  5.2
#define OPENFIRE_CODENAME "Dawn"
#define OPENFIRE_BOARD    "generic"

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
        GunSettings::Preferences_t *pref = _gunSettings->get_preference();

        switch (_gunSettings->get_gun_mode()) {
            case GunSettings::GunMode_Calibration:
                _gunJoy->set_auto_trigger(0, 0);
                break;

            default:
                // _gunJoy->set_auto_trigger(pref->device.auto_trg_delay, pref->device.auto_trg_rpt_delay);
                break;
        }
    }

    void set_ffb(int8_t type = -1, int8_t power = -1) {
        GunSettings::Preferences_t *pref = _gunSettings->get_preference();
        // uint16_t                    hold_delay = constrain(pref->device.auto_trg_rpt_delay / 3, 50, 300);
        uint16_t hold_delay = 50;

        // if (type != -1)
        //     pref->device.recoil_type = type;

        // if (power != -1)
        //     pref->device.recoil_pwr = power;

        if (type == 0) {
            _gunFFB->setup(PIN_RUMBLE, hold_delay, _pixels, 0);
        } else {
            _gunFFB->setup(PIN_SOLENOID, hold_delay, _pixels, 0);
        }
        // _gunFFB->set_power(pref->device.recoil_pwr);
    }

    void setup() {
        // load settings
        _gunSettings->setup();
        bool is_cal_req = !_gunSettings->load();
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

        // hid setup
        // _gunHID = new GunHIDBLE("OpenFIRE", "FIRECon", 0xF143, 0x1998);
        _gunHID = new GunHIDUSB("OpenFIRE", "FIRECon", 0xF143, 0x1998);
        _gunHID->setup();

        // ffb setup
        set_ffb();

        // camera setup
        _gunCam->setup(_gunSettings);

        // calibration setup
        _gunCali->setup(_gunSettings, _gunHID, _gunCam);
        if (is_cal_req) {
            LOGV("cali begin\n");
            _gunCali->begin();
        }

        _gunDock = new GunDock(_gunSettings, _gunHID->get_serial());
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

    void onCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) {
        switch (cmd) {
            case GunDock::CMD_IR_BRIGHTNESS: {
                uint8_t lvl = *pData - '0';
                LOGV("ir brightness : %d\n", lvl);
            }
            break;

            case GunDock::CMD_DOCK_MODE:
                LOGV("docked mode\n");
                _gunSettings->set_gun_mode(GunSettings::GunMode_Docked);
                stream->printf("OpenFIRE,%.1f,%s,%s,%i\r\n", OPENFIRE_VERSION, OPENFIRE_CODENAME, OPENFIRE_BOARD,
                               _gunSettings->get_cur_profile());
                break;

            case GunDock::CMD_EEPROM_READ_TOGGLES:
                LOGV("toggles\n");
                stream->printf("%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n", _gunSettings->_toggles.customPinsInUse,
                               _gunSettings->_toggles.rumbleActive, _gunSettings->_toggles.solenoidActive,
                               _gunSettings->_toggles.autofireActive, _gunSettings->_toggles.simpleMenu,
                               _gunSettings->_toggles.holdToPause, _gunSettings->_toggles.commonAnode,
                               _gunSettings->_toggles.lowButtonMode, _gunSettings->_toggles.rumbleFF);
                break;

            case GunDock::CMD_EEPROM_READ_PINS:
                LOGV("pins\n");
                stream->printf(
                    "%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n",
                    _gunSettings->_pins.bTrigger, _gunSettings->_pins.bGunA, _gunSettings->_pins.bGunB,
                    _gunSettings->_pins.bGunC, _gunSettings->_pins.bStart, _gunSettings->_pins.bSelect,
                    _gunSettings->_pins.bGunUp, _gunSettings->_pins.bGunDown, _gunSettings->_pins.bGunLeft,
                    _gunSettings->_pins.bGunRight, _gunSettings->_pins.bPedal, _gunSettings->_pins.bPedal2,
                    _gunSettings->_pins.bHome, _gunSettings->_pins.bPump, _gunSettings->_pins.oRumble,
                    _gunSettings->_pins.oSolenoid, _gunSettings->_pins.sRumble, _gunSettings->_pins.sSolenoid,
                    _gunSettings->_pins.sAutofire, _gunSettings->_pins.oPixel, _gunSettings->_pins.oLedR,
                    _gunSettings->_pins.oLedG, _gunSettings->_pins.oLedB, _gunSettings->_pins.pCamSDA,
                    _gunSettings->_pins.pCamSCL, _gunSettings->_pins.pPeriphSDA, _gunSettings->_pins.pPeriphSCL,
                    _gunSettings->_pins.aBattRead, _gunSettings->_pins.aStickX, _gunSettings->_pins.aStickY,
                    _gunSettings->_pins.aTMP36);
                break;

            case GunDock::CMD_EEPROM_READ_SETTINGS:
                LOGV("settings\n");
                stream->printf("%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n", _gunSettings->_settings.rumbleIntensity,
                               _gunSettings->_settings.rumbleInterval, _gunSettings->_settings.solenoidNormalInterval,
                               _gunSettings->_settings.solenoidFastInterval,
                               _gunSettings->_settings.solenoidLongInterval, _gunSettings->_settings.autofireWaitFactor,
                               _gunSettings->_settings.pauseHoldLength, _gunSettings->_settings.customLEDcount,
                               _gunSettings->_settings.customLEDstatic, _gunSettings->_settings.customLEDcolor1,
                               _gunSettings->_settings.customLEDcolor2, _gunSettings->_settings.customLEDcolor3);
                break;

            case GunDock::CMD_EEPROM_READ_USB:
                LOGV("usb PID\n");
                if (_gunSettings->_usb.deviceName[0] == '\0')
                    stream->printf("%i,SERIALREADERR01\r\n", _gunSettings->_usb.devicePID);
                else
                    stream->printf("%i,%s\r\n", _gunSettings->_usb.devicePID, _gunSettings->_usb.deviceName);
                break;

            case GunDock::CMD_EEPROM_READ_PROFILE:
                if (*pData >= '0' && *pData <= '3') {
                    uint8_t idx = *pData - '0';

                    LOGV("profile : %d\n", idx);
                    stream->printf(
                        "%i,%i,%i,%i,%.2f,%.2f,%i,%i,%i,%i,%s\r\n", _gunSettings->get_profile_data(idx)->topOffset,
                        _gunSettings->get_profile_data(idx)->bottomOffset, _gunSettings->get_profile_data(idx)->leftOffset,
                        _gunSettings->get_profile_data(idx)->rightOffset, _gunSettings->get_profile_data(idx)->TLled,
                        _gunSettings->get_profile_data(idx)->TRled, _gunSettings->get_profile_data(idx)->irSensitivity,
                        _gunSettings->get_profile_data(idx)->runMode, _gunSettings->get_profile_data(idx)->irLayout,
                        _gunSettings->get_profile_data(idx)->color, _gunSettings->get_profile_data(idx)->name);
                }
                break;
        }
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
