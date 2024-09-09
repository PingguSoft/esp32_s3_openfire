#include "GunSettings.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "debug.h"

// 4 byte header ID
const GunSettings::header_id_t GunSettings::_header_id = {'O', 'F', '0', '1'};

GunSettings::preferences_t GunSettings::_preferences = {
    MAX_PROFILE_CNT,
    0,
    {{0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, GunSettings::BtnMask_A,
      true, 0xFF0000, "Profile A"},
     {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, GunSettings::BtnMask_B,
      true, 0x00FF00, "Profile B"},
     {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, GunSettings::BtnMask_Start,
      true, 0x0000FF, "Profile Start"},
     {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, GunSettings::BtnMask_Select,
      true, 0xFF00FF, "Profile Select"}},
};

GunSettings::toggle_map_ut GunSettings::_toggles = {false, true, true, false, false, false, true, false, false};

GunSettings::pins_map_ut GunSettings::_pins = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

GunSettings::settings_map_t GunSettings::_settings = {255,  150, 45, 30,       500,      3,
                                                      2500, 1,   0,  0xFF0000, 0x00FF00, 0x0000FF};

GunSettings::usb_map_t GunSettings::_usb = {
    "SERIALREADERR01",
    0,
};

void GunSettings::setup() {
    EEPROM.begin(2048);
}

void GunSettings::save() {
    EEPROM.put(0, _header_id.u32);
    EEPROM.put(4, _preferences);
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

    hex2string((uint8_t *)&_preferences, sizeof(_preferences), buf);
    LOGV("%s\n", buf);

    free(buf);
}

bool GunSettings::load() {
    uint32_t u32;

    EEPROM.get(0, u32);
    if (u32 == _header_id.u32) {
        EEPROM.get(4, _preferences);
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

void GunSettings::onCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) {
    switch (cmd) {
        case GunDock::CMD_IR_BRIGHTNESS:
            get_profile_data()->irSensitivity = *pData - '0';
            break;

        case GunDock::CMD_DOCK_MODE:
            LOGV("docked mode\n");
            set_gun_mode(GunSettings::GunMode_Docked);
            stream->printf("OpenFIRE,%.1f,%s,%s,%i\r\n", OPENFIRE_VERSION, OPENFIRE_CODENAME, OPENFIRE_BOARD,
                           get_cur_profile());
            break;

        case GunDock::CMD_EEPROM_READ_TOGGLES: {
            LOGV("toggles\n");
            stream->printf("%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n", _toggles.s.customPinsInUse, _toggles.s.rumbleActive,
                           _toggles.s.solenoidActive, _toggles.s.autofireActive, _toggles.s.simpleMenu,
                           _toggles.s.holdToPause, _toggles.s.commonAnode, _toggles.s.lowButtonMode, _toggles.s.rumbleFF);
        } break;

        case GunDock::CMD_EEPROM_READ_PINS: {
            LOGV("pins\n");
            stream->printf(
                "%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n",
                _pins.s.bTrigger, _pins.s.bGunA, _pins.s.bGunB, _pins.s.bGunC, _pins.s.bStart, _pins.s.bSelect, _pins.s.bGunUp,
                _pins.s.bGunDown, _pins.s.bGunLeft, _pins.s.bGunRight, _pins.s.bPedal, _pins.s.bPedal2, _pins.s.bHome,
                _pins.s.bPump, _pins.s.oRumble, _pins.s.oSolenoid, _pins.s.sRumble, _pins.s.sSolenoid, _pins.s.sAutofire,
                _pins.s.oPixel, _pins.s.oLedR, _pins.s.oLedG, _pins.s.oLedB, _pins.s.pCamSDA, _pins.s.pCamSCL, _pins.s.pPeriphSDA,
                _pins.s.pPeriphSCL, _pins.s.aBattRead, _pins.s.aStickX, _pins.s.aStickY, _pins.s.aTMP36);
        } break;

        case GunDock::CMD_EEPROM_READ_SETTINGS:
            LOGV("settings\n");
            stream->printf("%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n", _settings.rumbleIntensity,
                           _settings.rumbleInterval, _settings.solenoidNormalInterval, _settings.solenoidFastInterval,
                           _settings.solenoidLongInterval, _settings.autofireWaitFactor, _settings.pauseHoldLength,
                           _settings.customLEDcount, _settings.customLEDstatic, _settings.customLEDcolor1,
                           _settings.customLEDcolor2, _settings.customLEDcolor3);
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
                stream->printf("%i,%i,%i,%i,%.2f,%.2f,%i,%i,%i,%i,%s\r\n", get_profile_data(idx)->topOffset,
                               get_profile_data(idx)->bottomOffset, get_profile_data(idx)->leftOffset,
                               get_profile_data(idx)->rightOffset, get_profile_data(idx)->TLled,
                               get_profile_data(idx)->TRled, get_profile_data(idx)->irSensitivity,
                               get_profile_data(idx)->runMode, get_profile_data(idx)->irLayout,
                               get_profile_data(idx)->color, get_profile_data(idx)->name);
            }
            break;

        case GunDock::CMD_TOGGLE_TEST_MODE:
            if (get_profile_data()->runMode == RunMode_Processing) {
                get_profile_data()->runMode = _runmode_saved;
                stream->printf("Exiting processing mode...\r\n");
            } else {
                _runmode_saved              = get_profile_data()->runMode;
                get_profile_data()->runMode = RunMode_Processing;
                stream->printf("Entering Test Mode...\r\n");
            }
            break;

        case GunDock::CMD_CALIBRATION_MODE: {
            // ch = '1' ~ '4'
            uint8_t idx = *pData - '0' - 1;
            set_cur_profile(idx);
            stream->printf("Profile:  %d\r\n", idx);
            set_gun_mode(GunMode_Calibration);
        } break;

        case GunDock::CMD_SAVE_PREFERENCE: {
            stream->printf("Saving preferences...\r\n");
        } break;

        case GunDock::CMD_CLEAR_EEPROM: {
            stream->printf("Cleared! Please reset the board.\r\n");
        } break;

        // "Xm.0.%1.%2", "Xm.1.%1.%2", "Xm.2.%1.%2", "Xm.3.0.%1", "Xm.3.1.%1"
        // "Xm.P.i.%1.%2", "Xm.P.r.%1.%2", "Xm.P.l.%1.%2", "Xm.P.c.%1.%2", "Xm.P.n.%1.%2"
        case GunDock::CMD_EEPROM_UPDATE: {
            LOGV("[%d] %s\n", size, (char *)pData);

            std::vector<char *> tokens;
            tokenize((char *)pData, (char*)".", tokens);

            uint8_t sub_cmd = tokens[0][0];
            uint8_t p1      = atoi(tokens[1]);
            uint8_t p2      = atoi(tokens[2]);
            switch (sub_cmd) {
                case '0':  // toggles
                    _toggles.arr[p1] = (bool)p2;
                    LOGV("toggle [%d]=%d\n", p1, p2);
                    break;

                case '1':  // pins
                    _pins.arr[p1] = p2;
                    LOGV("pin [%d]=%d\n", p1, p2);
                    break;

                case '2':  // settings
                    break;

                case '3':  //
                    break;

                case 'P':  // profile data
                    break;
            }
        } break;
    }
}