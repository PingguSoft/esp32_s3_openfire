#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <arduino-timer.h>

#include "GunCalibration.h"
#include "GunCamera.h"
#include "GunMenu.h"
#include "GunDisplay.h"
#include "GunDock.h"
#include "GunFFB.h"
#include "GunHID.h"
#include "GunJoyButton.h"
#include "GunMameHooker.h"
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
    uint32_t pad_evt;
    uint16_t dock_evt;
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
// clang-format off
static const pin_info_t _tbl_sw_pins[] = {
    {PIN_BUTTON_START,  INPUT_PULLUP,             0,  GUN_BTN_START,    5},
    {PIN_BUTTON_SELECT, INPUT_PULLUP,             0,  GUN_BTN_SELECT,   6},
    {PIN_BUTTON_PEDAL,  INPUT_PULLUP, MOUSE_FORWARD,  GUN_BTN_PEDAL,   11},
    {PIN_TRIGGER,       INPUT_PULLUP, MOUSE_LEFT,     GUN_BTN_TRIGGER,  1},
    {PIN_BUTTON_A,      INPUT_PULLUP, MOUSE_RIGHT,    GUN_BTN_A,        2},
    {PIN_BUTTON_B,      INPUT_PULLUP, MOUSE_MIDDLE,   GUN_BTN_B,        3},
    {PIN_BUTTON_C,      INPUT_PULLUP, MOUSE_BACKWARD, GUN_BTN_C,        4},

    {PIN_JOY_ADC_Y,     ANALOG,                    0, PAD_AXIS_Y | (JOY_ADC_MAX << 12) | (JOY_ADC_MIN), ( 7 << 8) | (8 << 0)},     // up-7, down-8
    {PIN_JOY_ADC_X,     ANALOG,                    0, PAD_AXIS_X | (JOY_ADC_MAX << 12) | (JOY_ADC_MIN), (10 << 8) | (9 << 0)},     // right-10, left-9
};

static const uint16_t IDM_PROFILE          = 0x0100;
static const uint16_t IDM_PROFILE_1        = 0x0101;
static const uint16_t IDM_PROFILE_2        = 0x0102;
static const uint16_t IDM_PROFILE_3        = 0x0103;
static const uint16_t IDM_PROFILE_4        = 0x0104;
static const uint16_t IDM_RUNMODE          = 0x0200;
static const uint16_t IDM_RUNMODE_NORM     = 0x0201;
static const uint16_t IDM_RUNMODE_AVG      = 0x0202;
static const uint16_t IDM_IR               = 0x0300;
static const uint16_t IDM_OFFSCR_BTN       = 0x0400;
static const uint16_t IDM_RUMBLE           = 0x0500;
static const uint16_t IDM_RUMBLE_EN        = 0x0501;
static const uint16_t IDM_RUMBLE_POWER     = 0x0502;
static const uint16_t IDM_RUMBLE_INTERVAL  = 0x0503;
static const uint16_t IDM_SOLENOID         = 0x0600;
static const uint16_t IDM_SOLENOID_EN      = 0x0601;
static const uint16_t IDM_SOLENOID_POWER   = 0x0602;
static const uint16_t IDM_SOLENOID_HOLD    = 0x0603;
static const uint16_t IDM_AUTOFIRE         = 0x0700;
static const uint16_t IDM_AUTOFIRE_EN      = 0x0701;
static const uint16_t IDM_AUTOFIRE_ST_DLY  = 0x0702;
static const uint16_t IDM_AUTOFIRE_RPT_DLY = 0x0702;
static const uint16_t IDM_CALIBRATION      = 0x0800;
static const uint16_t IDM_CALIBRATION_MSG  = 0x0801;
static const uint16_t IDM_INTERFACE        = 0x0900;
static const uint16_t IDM_INTERFACE_USB    = 0x0901;
static const uint16_t IDM_INTERFACE_BT     = 0x0902;
static const uint16_t IDM_SERIAL_MODE      = 0x0A00;
static const uint16_t IDM_SERIAL_DOCK      = 0x0A01;
static const uint16_t IDM_SERIAL_MAMEHOOK  = 0x0A02;
static const uint16_t IDM_SAVE             = 0x0E00;
static const uint16_t IDM_EXIT             = 0x0F00;

/*
*****************************************************************************************
* VARIABLES
*****************************************************************************************
*/
static std::vector<GunMenu::menu_item> _menu_sub_profile = {
    {IDM_PROFILE_1, "", GunMenu::TYPE_NORM_STR },
    {IDM_PROFILE_2, "", GunMenu::TYPE_NORM_STR },
    {IDM_PROFILE_3, "", GunMenu::TYPE_NORM_STR },
    {IDM_PROFILE_4, "", GunMenu::TYPE_NORM_STR }
};

static std::vector<GunMenu::menu_item> _menu_sub_runmode = {
    {IDM_RUNMODE_NORM, "Normal", GunMenu::TYPE_NORM_STR },
    {IDM_RUNMODE_AVG, "Average", GunMenu::TYPE_NORM_STR },
};

static std::vector<GunMenu::menu_item> _menu_sub_rumble = {
    {IDM_RUMBLE_EN, "Enable", GunMenu::TYPE_BOOL, },
    {IDM_RUMBLE_POWER, "Intensity", GunMenu::TYPE_DIGIT_8 },
    {IDM_RUMBLE_INTERVAL, "Interval", GunMenu::TYPE_DIGIT_16 },
};
static std::vector<GunMenu::menu_item> _menu_sub_solenoid = {
    {IDM_SOLENOID_EN, "Enable", GunMenu::TYPE_BOOL },
    {IDM_SOLENOID_POWER, "Intensity", GunMenu::TYPE_DIGIT_8 },
    {IDM_SOLENOID_HOLD, "Hold Delay", GunMenu::TYPE_DIGIT_16 },
};

static std::vector<GunMenu::menu_item> _menu_sub_autofire = {
    {IDM_AUTOFIRE_EN, "Enable", GunMenu::TYPE_BOOL },
    {IDM_AUTOFIRE_ST_DLY, "Strt Delay", GunMenu::TYPE_DIGIT_16 },
    {IDM_AUTOFIRE_RPT_DLY, "Rpt Delay", GunMenu::TYPE_DIGIT_16 },
};

static std::vector<GunMenu::menu_item> _menu_sub_cal = {
    {IDM_CALIBRATION_MSG, "Welcome!\nPull trigger to\nstart calibration!", GunMenu::TYPE_CENTER_STR}
};

static std::vector<GunMenu::menu_item> _menu_sub_if = {
    {IDM_INTERFACE_USB, "USB", GunMenu::TYPE_NORM_STR},
    {IDM_INTERFACE_BT , "Bluetooth", GunMenu::TYPE_NORM_STR}
};

static std::vector<GunMenu::menu_item> _menu_sub_serial = {
    {IDM_SERIAL_DOCK, "Dock", GunMenu::TYPE_NORM_STR},
    {IDM_SERIAL_MAMEHOOK , "MameHooker", GunMenu::TYPE_NORM_STR}
};

static std::vector<GunMenu::menu_item> _menu_top = {
    {IDM_PROFILE, "PROFILE", GunMenu::TYPE_NORM_STR, &_menu_sub_profile },
    {IDM_RUNMODE, "RUN MODE", GunMenu::TYPE_NORM_STR, &_menu_sub_runmode },
    {IDM_IR, "IR SENSITIVITY", GunMenu::TYPE_DIGIT_8 },
    {IDM_OFFSCR_BTN, "OFFSCRN BUTTON", GunMenu::TYPE_BOOL },
    {IDM_RUMBLE, "RUMBLE",GunMenu::TYPE_NORM_STR, &_menu_sub_rumble },
    {IDM_SOLENOID, "SOLENOID", GunMenu::TYPE_NORM_STR, &_menu_sub_solenoid },
    {IDM_AUTOFIRE, "AUTO FIRE", GunMenu::TYPE_NORM_STR, &_menu_sub_autofire },
    {IDM_CALIBRATION, "CALIBRATION", GunMenu::TYPE_NORM_STR, &_menu_sub_cal },
    {IDM_INTERFACE, "INTERFACE", GunMenu::TYPE_NORM_STR, &_menu_sub_if },
    {IDM_SERIAL_MODE, "SERIAL PORT", GunMenu::TYPE_NORM_STR, &_menu_sub_serial },
    {IDM_SAVE, "SAVE", GunMenu::TYPE_NORM_STR },
    {IDM_EXIT, "EXIT", GunMenu::TYPE_NORM_STR },
};
// clang-format on

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


//
// GunMenuHandler
//
class GunMenuHandler {
   public:
    GunMenuHandler() : _enable(true) {}

    void onMenuItemInit(uint16_t id, GunMenu::menu_item *item) {
        switch (id) {
            case IDM_PROFILE_1:
            case IDM_PROFILE_2:
            case IDM_PROFILE_3:
            case IDM_PROFILE_4:
                item->set_name(_settings->get_profile(id - IDM_PROFILE_1)->name);
                break;
        }
    }

    void onMenuItemClicked(uint16_t id, GunMenu::menu_item *item) {
        switch (id) {
            case IDM_PROFILE:
                _menu.set_pos(_settings->get_profile_idx());
                break;

            case IDM_PROFILE_1:
            case IDM_PROFILE_2:
            case IDM_PROFILE_3:
            case IDM_PROFILE_4:
                _settings->set_profile_idx(id - IDM_PROFILE_1);
                break;

            case IDM_RUNMODE:
                _menu.set_pos(_settings->get_profile()->runMode);
                break;

            case IDM_RUNMODE_NORM:
            case IDM_RUNMODE_AVG:
                _settings->get_profile()->runMode = (id - IDM_RUNMODE_NORM);
                break;

            case IDM_SERIAL_DOCK:
                _settings->get_feature_config()->serialHooker = false;
                break;

            case IDM_SERIAL_MAMEHOOK:
                _settings->get_feature_config()->serialHooker = true;
                break;
        }
    }

    void setup(GunSettings *settings, GunDisplay *disp, GunMenu::Callback *callback) {
        // clang-format off
        std::map<uint16_t, GunMenu::item_meta *> bind_tbl = {
            {IDM_IR,               new GunMenu::item_meta(&_settings->get_profile()->irSensitivity, 0, 3)},
            {IDM_OFFSCR_BTN,       new GunMenu::item_meta(&_settings->get_feature_config()->lowButtonMode)},
            {IDM_RUMBLE_EN,        new GunMenu::item_meta(&_settings->get_feature_config()->rumbleActive)},
            {IDM_RUMBLE_POWER,     new GunMenu::item_meta(&_settings->get_param_config()->rumbleIntensity, 0, 255, 3)},
            {IDM_RUMBLE_INTERVAL,  new GunMenu::item_meta(&_settings->get_param_config()->rumbleInterval, 50, 500, 3, 5)},
            {IDM_SOLENOID_EN,      new GunMenu::item_meta(&_settings->get_feature_config()->solenoidActive)},
            {IDM_SOLENOID_POWER,   new GunMenu::item_meta(&_settings->get_param_config()->solenoidIntensity, 0, 255, 3)},
            {IDM_SOLENOID_HOLD,    new GunMenu::item_meta(&_settings->get_param_config()->solenoidNormalInterval, 40, 100, 3, 5)},
            {IDM_AUTOFIRE_EN,      new GunMenu::item_meta(&_settings->get_feature_config()->autofireActive)},
            {IDM_AUTOFIRE_ST_DLY,  new GunMenu::item_meta(&_settings->get_param_config()->solenoidLongInterval,  50, 500, 3, 5)},
            {IDM_AUTOFIRE_RPT_DLY, new GunMenu::item_meta(&_settings->get_param_config()->solenoidFastInterval, 40, 100, 3, 5)},
        };
        // clang-format on
        _menu.setup("SETTINGS", &_menu_top, callback, &bind_tbl);
        _settings = settings;
        _disp = disp;
    }

    void loop(uint32_t btns) {
        if (!_enable)
            return;

        _btn_trk.begin(btns);
        if (_btn_trk.isPressed(GUN_BTN_START) || _btn_trk.isPressed(PAD_HAT_MASK_Y_P << 24)) {
            _menu.handle_event(GunMenu::KEY_UP);
        } else if (_btn_trk.isPressed(GUN_BTN_SELECT) || _btn_trk.isPressed(PAD_HAT_MASK_Y_M << 24)) {
            _menu.handle_event(GunMenu::KEY_DOWN);
        } else if (_btn_trk.isPressed(GUN_BTN_A) || _btn_trk.isPressed(PAD_HAT_MASK_X_P << 24)) {
            _menu.handle_event(GunMenu::KEY_RIGHT);
        } else if (_btn_trk.isPressed(GUN_BTN_B) || _btn_trk.isPressed(PAD_HAT_MASK_X_M << 24)) {
            _menu.handle_event(GunMenu::KEY_LEFT);
        }

        if (_btn_trk.isPressed(GUN_BTN_TRIGGER)) {
            _menu.handle_event(GunMenu::KEY_ENTER);
        } else if (_btn_trk.isPressed(GUN_BTN_PEDAL)) {
            _menu.handle_event(GunMenu::KEY_BACK);
        }
        _btn_trk.end();

        if (_menu.updated())
            _menu.draw(_disp);
    }

    void jump(uint16_t id) {
        _menu.jump(id);
        if (_menu.updated())
            _menu.draw(_disp);
    }

    void enable(bool en) { _enable = en; }

   private:
    bool          _enable;
    GunMenu       _menu;
    GunDisplay   *_disp;
    GunSettings  *_settings;
    ButtonTracker _btn_trk;
};


//
// GunMain
//
class GunMain : public GunSerialCallback, public GunMenu::Callback {
   public:
    GunMain() {
        _gunJoy      = new GunJoyButton();
        _gunFFB      = new GunFFB();
        _gunCam      = new GunCamera();
        _gunCali     = new GunCalibration();
        _gunSettings = new GunSettings();
        _pixels      = new Adafruit_NeoPixel(1, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);
        _gunDisp     = new GunDisplay();
        _gunMenu     = new GunMenuHandler();
    }

    void onMenuItemInit(uint16_t id, GunMenu::menu_item *item) {
        _gunMenu->onMenuItemInit(id, item);
    }

    void onMenuItemClicked(uint16_t id, GunMenu::menu_item *item) {
        _gunMenu->onMenuItemClicked(id, item);

        switch (id) {
            case IDM_CALIBRATION_MSG:
                update_gun_mode(GunSettings::GunMode_Calibration);
                break;

            case IDM_IR:
                _gunCam->update_setting();
                break;

            case IDM_SERIAL_DOCK:
            case IDM_SERIAL_MAMEHOOK:
                if (_gunSerial)
                    delete _gunSerial;
                if (_gunSettings->get_feature_config()->serialHooker)
                    _gunSerial = new GunMameHooker(_gunHID->get_serial());
                else
                    _gunSerial = new GunDock(_gunHID->get_serial());
                break;
        }
    }

    void onMameHookCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) {
        switch (cmd) {
            case GunMameHooker::CMD_START: {
                uint8_t mode = *pData;
                if (mode == '0') {              // Start with solenoid enabled
                } else if (mode == '1') {       // Start with rumble enabled
                } else if (mode == '2') {       // Start with the RED LED enabled
                } else if (mode == '3') {       // Start with the GREEN LED enabled
                } else if (mode == '4') {       // Start with the BLUE LED enabled
                } else if (mode == '6') {       // Start with everything enabled
                }
            } break;

            case GunMameHooker::CMD_END:
                break;

            case GunMameHooker::CMD_MODE: {
                uint8_t mode = *pData;
                uint8_t sub  = *(pData + 2);

                switch (mode) {
                    case '0':   // Device Output Mode
                        if (sub == '0') {           // mouse & keyboard
                        } else if (sub == '1') {    // Gamepad, w/ Camera mapped to Right Stick
                        } else if (sub == '2') {    // hybrid
                        }
                        break;

                    case '1':   // Offscreen Firing Mode
                        if (sub == '0') {           // Disabled (not used in OpenFIRE)
                        } else if (sub == '1') {    // Fire in bottom-left corner (not used in OpenFIRE)
                        } else if (sub == '2') {    // Offscreen Button Mode enabled (i.e. offscreen trigger pulls generates a Right Click instead of a Left Click)
                        } else if (sub == '3') {    // Normal shot (always on when Offscreen Button Mode isn't set in OpenFIRE)
                        }
                        break;

                    case '2':   // Pedal Mapping
                        if (sub == '0') {           // Separate Button (as mapped)
                        } else if (sub == '1') {    // As Right Mouse
                        } else if (sub == '2') {    // As Middle Mouse (OpenFIRE exclusive)
                        }
                        break;

                    case '3':  // Aspect Ratio correction
                        if (sub == '0') {           // Fullscreen
                        } else if (sub == '1') {    // 4:3 correction
                        }
                        break;

                    case '4':  // Temp Sensor Control (not used in OpenFIRE)
                        if (sub == '0') {           // disabled
                        } else if (sub == '1') {    // enabled
                        }
                        break;

                    case '5':  // Auto Reload (not used in OpenFIRE)
                        if (sub == '0') {           // disabled
                        } else if (sub == '1') {    // enabled
                        }
                        break;

                    case '6':  // Rumble Only Mode
                        if (sub == '0') {           // Disabled (Solenoid allowed)
                        } else if (sub == '1') {    // Enabled (Solenoid disabled, Rumble enabled)
                        }
                        break;

                    case '8':  // Autofire Mode
                        if (sub == '0') {           // Disabled (sustained fire is kept enabled in OpenFIRE)
                        } else if (sub == '1') {    // Auto fire on (enables Burst Fire in OpenFIRE)
                        } else if (sub == '2') {    // Auto fire always on rapid fire
                        }
                        break;

                    case 'D':  // Display Mode (OpenFIRE exclusive)
                        if (sub == 1) {             // Life Only
                        } else if (sub == '2') {    // Ammo Only
                        } else if (sub == '3') {    // Life & Ammo Splitscreen
                            if (size > 3 && *(pData + 3) == 'B') {  // Life Bar (Life Glyphs otherwise)
                            }
                        }
                        break;
                }
            } break;

            case GunMameHooker::CMD_FFB:{
                uint8_t mode = *pData;
                uint8_t sub  = *(pData + 2);

                switch (mode) {
                    case '0':   // Solenoid
                        if (sub == '0') {           // Off
                        } else if (sub == '1') {    // On
                        } else if (sub == '2') {    // Pulse
                            atoi((char*)(pData + 4));
                        }
                        break;

                    case '1':   // Rumble
                        if (sub == '0') {           // Off
                        } else if (sub == '1') {    // On
                        } else if (sub == '2') {    // Pulse
                            atoi((char*)(pData + 4));
                        }
                        break;

                    case '2':   // RGB Red
                    case '3':   // RGB Green
                    case '4':   // RGB Blue
                        if (sub == '0') {           // Off
                        } else if (sub == '1') {    // On
                            atoi((char*)(pData + 4));
                        } else if (sub == '2') {    // Pulse
                            atoi((char*)(pData + 4));
                        }
                        break;

                    case 'D':   // Display Event (OpenFIRE exclusive)
                        if (*(pData + 1) == 'A') {          // New Ammo Count
                            atoi((char*)(pData + 3));
                        } else if (*(pData + 1) == 'L') {   // New Life Count
                            atoi((char*)(pData + 3));
                        }
                        break;
                }
                break;
            } break;
        }
    }

    void onDockCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) {
        static GunSettings::GunMode_e last_mode;

        _gunSettings->onSerialCallback(cmd, pData, size, stream);
        switch (cmd) {
            case GunDock::CMD_DOCK_MODE:
                last_mode = update_gun_mode(*pData ? GunSettings::GunMode_Docked : last_mode);
                break;

            case GunDock::CMD_CALIBRATION_MODE:
                update_gun_mode(GunSettings::GunMode_Calibration);
                break;

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

            default:
                break;
        }
    }

    void onSerialCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) {
        if (_gunSettings->get_feature_config()->serialHooker)
            onMameHookCallback(cmd, pData, size, stream);
        else
            onDockCallback(cmd, pData, size, stream);
    }

    void update_auto_fire(GunSettings::GunMode_e mode) {
        uint16_t strt_dly;
        uint16_t rpt_dly;

        switch (mode) {
            case GunSettings::GunMode_Calibration:
                strt_dly = 0;
                rpt_dly  = 0;
                break;

            case GunSettings::GunMode_Pause:
                strt_dly = 300;
                rpt_dly  = 200;
                break;

            default:
                if (_gunSettings->get_feature_config()->autofireActive) {
                    strt_dly = _gunSettings->get_param_config()->solenoidLongInterval;
                    rpt_dly  = _gunSettings->get_param_config()->solenoidFastInterval;
                } else {
                    strt_dly = 0;
                    rpt_dly  = 0;
                }
                break;
        }
        _gunJoy->set_auto_trigger(strt_dly, rpt_dly);
    }

    void update_force_feedback(uint8_t mode = 0) {
        GunSettings::params_map_t *params = _gunSettings->get_param_config();

        if (mode == 's' || (mode == 0 && _gunSettings->get_feature_config()->solenoidActive)) {
            _gunFFB->setup(PIN_SOLENOID, params->solenoidFastInterval, _pixels, 0);
        } else if (mode == 'r' || (mode == 0 && _gunSettings->get_feature_config()->rumbleActive)) {
            _gunFFB->setup(PIN_RUMBLE, params->solenoidFastInterval, _pixels, 0);
        }
        _gunFFB->set_power(params->solenoidIntensity);
    }

    GunSettings::GunMode_e update_gun_mode(GunSettings::GunMode_e mode) {
        GunSettings::GunMode_e last = _gunSettings->get_gun_mode();

        LOGV("gun mode : %d->%d\n", last, mode);
        _gunSettings->set_gun_mode(mode);
        update_auto_fire(mode);
        update_force_feedback();
        if (mode == GunSettings::GunMode_Calibration) {
            _gunMenu->jump(IDM_CALIBRATION_MSG);
            _gunCali->begin(last);
        }
        return last;
    }

    void setup() {
        // load settings
        bool is_loaded = _gunSettings->setup();

        // pixels
        _pixels->begin();
        _pixels->show();
        _pixels->setBrightness(50);
        _pixels->setPixelColor(0, _pixels->Color(0, 0, 255));
        _pixels->show();

        // oled display
        pinMode(PIN_PERI_SDA, INPUT_PULLUP);
        pinMode(PIN_PERI_SCL, INPUT_PULLUP);
        Wire1.begin(PIN_PERI_SDA, PIN_PERI_SCL, 400000);
        _gunDisp->setup(&Wire1);
        _gunMenu->setup(_gunSettings, _gunDisp, this);

        // joypad setting
        for (int i = 0; i < ARRAY_SIZE(_tbl_sw_pins); i++) {
            _gunJoy->add_button(_tbl_sw_pins[i].pin, _tbl_sw_pins[i].mode, _tbl_sw_pins[i].mouse_evt,
                                _tbl_sw_pins[i].pad_evt);
        }
        _gunJoy->setup();

        // hid setup
        // _gunHID = new GunHIDBLE("OpenFIRE", "FIRECon", 0xF143, 0x1998);
        _gunHID = new GunHIDUSB("OpenFIRE", "FIRECon", 0xF143, _gunSettings->get_usb_config()->devicePID);
        _gunHID->setup();

        // camera setup
        _gunCam->setup(_gunSettings);

        // serial setup
        if (_gunSettings->get_feature_config()->serialHooker)
            _gunSerial = new GunMameHooker(_gunHID->get_serial(), this);
        else
            _gunSerial = new GunDock(_gunHID->get_serial(), this);

        // calibration setup
        _gunCali->setup(_gunSettings, _gunHID, _gunCam);

        // initial gun mode
        update_gun_mode(is_loaded ? GunSettings::GunMode_Run : GunSettings::GunMode_Calibration);
    }

    void handle_ir_test(GunSettings *settings, GunCamera *cam) {
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
        if (!_gunSettings->get_feature_config()->serialHooker)
            _gunSerial->get_stream()->print(buf);
    }

    void handle_event(GunJoyButton::report_t *report) {
        int8_t x;
        int8_t y;

        // mode changed ?
        GunSettings::GunMode_e mode = _gunSettings->get_gun_mode();
        uint32_t               btns = ((int)_gunJoy->get_hat_mask() << 24) | report->pad_buttons;

        if (mode != _last_mode) {
            LOGV("gunmode is changed : %d -> %d\n", _last_mode, mode);
            _last_mode = mode;
        }

        _btn_trk.begin(btns);
        switch (mode) {
            case GunSettings::GunMode_Init:
                break;

            case GunSettings::GunMode_Calibration:
            case GunSettings::GunMode_Verification: {
                GunSettings::GunMode_e ret = _gunCali->loop(report->pad_buttons, mode);
                if (ret != mode) {
                    if (ret == GunSettings::GunMode_Init)
                        ret = _gunCali->end();
                    update_gun_mode(ret);
                }
            } break;

            case GunSettings::GunMode_Docked:
                for (int i = 0; i < ARRAY_SIZE(_tbl_sw_pins); i++) {
                    if (_tbl_sw_pins[i].mode == ANALOG) {
                        int mask1, mask2;

                        uint8_t evt1 = _tbl_sw_pins[i].dock_evt >> 8;
                        uint8_t evt2 = _tbl_sw_pins[i].dock_evt & 0xff;

                        if (_tbl_sw_pins[i].pad_evt & PAD_AXIS_X) {
                            mask1 = PAD_HAT_MASK_X_P << 24;
                            mask2 = PAD_HAT_MASK_X_M << 24;
                        } else if (_tbl_sw_pins[i].pad_evt & PAD_AXIS_Y) {
                            mask1 = PAD_HAT_MASK_Y_P << 24;
                            mask2 = PAD_HAT_MASK_Y_M << 24;
                        }

                        if (!_gunSettings->get_feature_config()->serialHooker) {
                            if (_btn_trk.isPressed(mask1)) {
                                _gunSerial->get_stream()->printf("Pressed: %d\r\n", evt1);
                            } else if (_btn_trk.isReleased(mask1)) {
                                _gunSerial->get_stream()->printf("Released: %d\r\n", evt1);
                            }

                            if (_btn_trk.isPressed(mask2)) {
                                _gunSerial->get_stream()->printf("Pressed: %d\r\n", evt2);
                            } else if (_btn_trk.isReleased(mask2)) {
                                _gunSerial->get_stream()->printf("Released: %d\r\n", evt2);
                            }
                        }
                    } else {
                        if (!_gunSettings->get_feature_config()->serialHooker) {
                            if (_btn_trk.isPressed(_tbl_sw_pins[i].pad_evt)) {
                                _gunSerial->get_stream()->printf("Pressed: %d\r\n", _tbl_sw_pins[i].dock_evt);
                            } else if (_btn_trk.isReleased(_tbl_sw_pins[i].pad_evt)) {
                                _gunSerial->get_stream()->printf("Released: %d\r\n", _tbl_sw_pins[i].dock_evt);
                            }
                        }
                    }
                }
                if (!_gunSettings->get_feature_config()->serialHooker)
                    _gunSerial->get_stream()->printf("Analog: %d\r\n", _tbl_hat2fire[report->hat]);

                x = map(_gunCam->x(), 0, GunHID::mouse_max_x, -127, 127);
                y = map(_gunCam->y(), 0, GunHID::mouse_max_y, -127, 127);
                _gunHID->report_gamepad(x, y, 0, report->pad_buttons);
                handle_ir_test(_gunSettings, _gunCam);
                break;

            case GunSettings::GunMode_Run:
                if (_btn_trk.isPressed(GUN_BTN_SELECT | GUN_BTN_PEDAL)) {
                    update_gun_mode(GunSettings::GunMode_Pause);
                } else {
                    // cam processing
                    _gunCam->loop();

                    if (_gunCam->avail()) {
                        _gunHID->report_mouse(_gunCam->x(), _gunCam->y(), report->mouse_buttons);
                    }
                    x = map(_gunCam->x(), 0, GunHID::mouse_max_x, -127, 127);
                    y = map(_gunCam->y(), 0, GunHID::mouse_max_y, -127, 127);
                    _gunHID->report_gamepad(x, y, 0, report->pad_buttons);

                    // force feed back processing
                    _gunFFB->loop();
                }
                break;

            case GunSettings::GunMode_Pause:
                _gunMenu->loop(btns);
                if (_btn_trk.isPressed(GUN_BTN_START | GUN_BTN_PEDAL)) {
                    update_gun_mode(GunSettings::GunMode_Run);
                }
                break;
        }
        _btn_trk.end();
    }

    void loop() {
        bool                    update;
        GunJoyButton::report_t *report;

        // process serial commands
        _gunSerial->process();

        // input button processing
        update = _gunJoy->loop();
        report = _gunJoy->get();
        if (update) {
            // LOGV("%5d, %5d, %2d %4x %2x\n", report->x, report->y, report->hat, report->pad_buttons,
            //      report->mouse_buttons);
        }

        handle_event(report);
    }

   private:
    Adafruit_NeoPixel *_pixels;
    GunHID            *_gunHID;
    GunJoyButton      *_gunJoy;
    GunFFB            *_gunFFB;
    GunFFB            *_gunRumble;
    GunCamera         *_gunCam;
    GunCalibration    *_gunCali;
    GunSettings       *_gunSettings;
    GunDisplay        *_gunDisp;
    GunMenuHandler    *_gunMenu;
    GunSerial         *_gunSerial;
    ButtonTracker      _btn_trk;
    GunSettings::GunMode_e _last_mode;
    uint8_t            _tbl_hat2fire[9] = {0, 1, 8, 7, 6, 5, 4, 3, 2};
};

GunMain *_main = new GunMain();

void setup() {
    Serial0.begin(115200);
    // Wire.begin(PIN_IR_SDA, PIN_IR_SCL, 400000);

    LOGV("Start !!!\n");
    _main->setup();
}

void loop() {
    _main->loop();
}
