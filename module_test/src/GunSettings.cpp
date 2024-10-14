#include "GunSettings.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "GunJoyButton.h"
#include "debug.h"


// 4 byte header ID
const GunSettings::header_id_t GunSettings::_header_id = {'O', 'F', '0', '1'};

GunSettings::profiles_t GunSettings::_profiles = {
    MAX_PROFILE_CNT,
    0,
    {{0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, PAD_BUTTON_A, true,
      0xFF0000, "Profile A"},
     {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, PAD_BUTTON_B, true,
      0x00FF00, "Profile B"},
     {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, PAD_BUTTON_START, true,
      0x0000FF, "Profile Start"},
     {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, PAD_BUTTON_SELECT, true,
      0xFF00FF, "Profile Select"}},
};

GunSettings::feature_map_t GunSettings::_features = {false, true, true, false, false, false, true, false, false};

GunSettings::pins_map_t GunSettings::_pins = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

GunSettings::params_map_t GunSettings::_params = {255, 150, 45, 30, 500, 3, 2500, 1, 0, 0xFF0000, 0x00FF00, 0x0000FF};

GunSettings::usb_map_t GunSettings::_usb = {
    "SERIALREADERR01",
    0x1998,
};

void GunSettings::setup() {
    EEPROM.begin(2048);
}

void GunSettings::save() {
    EEPROM.put(0, _header_id.u32);
    EEPROM.put(4, _profiles);
    EEPROM.commit();
}

int hex2string(uint8_t *in, int inlen, char *out) {
    int   i   = 0;
    char *pos = out;

    for (i = 0; i < inlen; i++) pos += sprintf(pos, "%02X ", in[i]);

    return pos - out + 1;
}

void GunSettings::dump() {
    char *buf = (char *)malloc(4096);

    hex2string((uint8_t *)&_profiles, sizeof(_profiles), buf);
    LOGV("%s\n", buf);

    free(buf);
}

bool GunSettings::load() {
    uint32_t u32;

    EEPROM.get(0, u32);
    if (u32 == _header_id.u32) {
        EEPROM.get(4, _profiles);
        LOGV("loaded\n");
        return true;
    }
    LOGV("load fail\n");
    return false;
}

void GunSettings::tokenize(char *line, char *token, std::vector<char *> &tokens) {
    char *tkn = strtok(line, token);

    while (tkn != NULL) {
        tokens.push_back(tkn);
        tkn = strtok(NULL, token);
    }
}

void GunSettings::onDockCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) {
    switch (cmd) {
        case GunDock::CMD_IR_BRIGHTNESS:
            get_profile()->irSensitivity = *pData - '0';
            break;

        case GunDock::CMD_DOCK_MODE:
            if (*pData) {
                LOGV("docked mode\n");
                set_gun_mode(GunSettings::GunMode_Docked);
                stream->printf("OpenFIRE,%.1f,%s,%s,%i\r\n", OPENFIRE_VERSION, OPENFIRE_CODENAME, OPENFIRE_BOARD,
                               get_profile_idx());
            } else {
                LOGV("docked mode exit\n");
                set_gun_mode(GunSettings::GunMode_Run);
            }
            break;

        case GunDock::CMD_EEPROM_READ_TOGGLES:
            LOGV("toggles\n");
            stream->printf("%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n", _features.customPinsInUse, _features.rumbleActive,
                           _features.solenoidActive, _features.autofireActive, _features.simpleMenu,
                           _features.holdToPause, _features.commonAnode, _features.lowButtonMode, _features.rumbleFF);
            break;

        case GunDock::CMD_EEPROM_READ_PINS:
            LOGV("pins\n");
            stream->printf(
                "%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n",
                _pins.bTrigger, _pins.bGunA, _pins.bGunB, _pins.bGunC, _pins.bStart, _pins.bSelect, _pins.bGunUp,
                _pins.bGunDown, _pins.bGunLeft, _pins.bGunRight, _pins.bPedal, _pins.bPedal2, _pins.bHome, _pins.bPump,
                _pins.oRumble, _pins.oSolenoid, _pins.sRumble, _pins.sSolenoid, _pins.sAutofire, _pins.oPixel,
                _pins.oLedR, _pins.oLedG, _pins.oLedB, _pins.pCamSDA, _pins.pCamSCL, _pins.pPeriphSDA, _pins.pPeriphSCL,
                _pins.aBattRead, _pins.aStickX, _pins.aStickY, _pins.aTMP36);
            break;

        case GunDock::CMD_EEPROM_READ_SETTINGS:
            LOGV("settings\n");
            stream->printf("%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n", _params.rumbleIntensity, _params.rumbleInterval,
                           _params.solenoidNormalInterval, _params.solenoidFastInterval, _params.solenoidLongInterval,
                           _params.autofireWaitFactor, _params.pauseHoldLength, _params.customLEDcount,
                           _params.customLEDstatic, _params.customLEDcolor1, _params.customLEDcolor2,
                           _params.customLEDcolor3);
            break;

        case GunDock::CMD_EEPROM_READ_USB:
            LOGV("usb PID\n");
            if (_usb.deviceName[0] == '\0')
                stream->printf("%i,SERIALREADERR01\r\n", _usb.devicePID);
            else
                stream->printf("%i,%s\r\n", _usb.devicePID, _usb.deviceName);
            break;

        case GunDock::CMD_EEPROM_READ_PROFILE:
            if (*pData >= '0' && *pData <= '3') {
                uint8_t idx = *pData - '0';

                LOGV("profile : %d\n", idx);
                stream->printf("%i,%i,%i,%i,%.2f,%.2f,%i,%i,%i,%i,%s\r\n", get_profile(idx)->topOffset,
                               get_profile(idx)->bottomOffset, get_profile(idx)->leftOffset,
                               get_profile(idx)->rightOffset, get_profile(idx)->TLled, get_profile(idx)->TRled,
                               get_profile(idx)->irSensitivity, get_profile(idx)->runMode, get_profile(idx)->irLayout,
                               get_profile(idx)->color, get_profile(idx)->name);
            }
            break;

        case GunDock::CMD_TOGGLE_TEST_MODE:
            if (get_profile()->runMode == RunMode_Processing) {
                get_profile()->runMode = _runmode_saved;
                stream->printf("Exiting processing mode...\r\n");
            } else {
                _runmode_saved         = get_profile()->runMode;
                get_profile()->runMode = RunMode_Processing;
                stream->printf("Entering Test Mode...\r\n");
            }
            break;

        case GunDock::CMD_CALIBRATION_MODE: {
            // ch = '1' ~ '4'
            uint8_t idx = *pData - '0' - 1;
            set_profile_idx(idx);
            stream->printf("Profile:  %d\r\n", idx);
            set_gun_mode(GunMode_Calibration);
        } break;

        case GunDock::CMD_SAVE_PREFERENCE:
            stream->printf("Saving preferences...\r\n");
            break;

        case GunDock::CMD_CLEAR_EEPROM:
            stream->printf("Cleared! Please reset the board.\r\n");
            break;

        // "Xm.0.%1.%2", "Xm.1.%1.%2", "Xm.2.%1.%2", "Xm.3.0.%1", "Xm.3.1.%1"
        // "Xm.P.i.%1.%2", "Xm.P.r.%1.%2", "Xm.P.l.%1.%2", "Xm.P.c.%1.%2", "Xm.P.n.%1.%2"
        case GunDock::CMD_EEPROM_UPDATE: {
            LOGV("[%d] %s\n", size, (char *)pData);

            std::vector<char *> tokens;
            tokenize((char *)pData, (char *)".", tokens);

            uint8_t  sub_cmd = tokens[0][0];
            uint8_t  p1      = atoi(tokens[1]);
            uint32_t p2      = atoi(tokens[2]);
            switch (sub_cmd) {
                case '0': {  // toggles
                    bool *arr = (bool *)&_features;

                    arr[p1] = (bool)p2;
                    LOGV("toggle [%d]=%d\n", p1, p2);
                } break;

                case '1': {  // pins
                    int8_t *arr = (int8_t *)&_pins;

                    arr[p1] = (int8_t)p2;
                    LOGV("pin [%d]=%d\n", p1, p2);
                } break;

                case '2':  // settings
                    switch (p1) {
                        case 0:
                            _params.rumbleIntensity = p2;
                            break;
                        case 1:
                            _params.rumbleIntensity = p2;
                            break;
                        case 2:
                            _params.rumbleInterval = p2;
                            break;
                        case 3:
                            _params.solenoidNormalInterval = p2;
                            break;
                        case 4:
                            _params.solenoidFastInterval = p2;
                            break;
                        case 5:
                            _params.solenoidLongInterval = p2;
                            break;
                        case 6:
                            _params.autofireWaitFactor = p2;
                            break;
                        case 7:
                            _params.pauseHoldLength = p2;
                            break;
                        case 8:
                            _params.customLEDcount = p2;
                            break;
                        case 9:
                            _params.customLEDstatic = p2;
                            break;
                        case 10:
                            _params.customLEDcolor1 = p2;
                            break;
                        case 11:
                            _params.customLEDcolor2 = p2;
                            break;
                        case 12:
                            _params.customLEDcolor3 = p2;
                            break;
                    }
                    break;

                case '3':  // usb
                    switch (p1) {
                        case 0:
                            _usb.devicePID = p2;
                            break;
                        case 1:
                            strncpy(_usb.deviceName, tokens[3], sizeof(_usb.deviceName));
                            break;
                    }
                    break;

                case 'P':  // profile data
                    p1 = atoi(tokens[2]);
                    p2 = atoi(tokens[3]);

                    switch (tokens[1][0]) {
                        case 'i':
                            get_profile(p1)->irSensitivity = p2;
                            break;
                        case 'r':
                            get_profile(p1)->runMode = p2;
                            break;
                        case 'l':
                            get_profile(p1)->irLayout = p2;
                            break;
                        case 'c':
                            get_profile(p1)->color = p2;
                            break;
                        case 'n':
                            strncpy(get_profile(p1)->name, tokens[3], sizeof(get_profile(p1)->name));
                            break;
                    }
                    break;
            }
        } break;
    }
}