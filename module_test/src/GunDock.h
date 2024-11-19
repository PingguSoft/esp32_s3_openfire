#ifndef __GUN_DOCK_H__
#define __GUN_DOCK_H__

#include <Arduino.h>

#include "debug.h"
#include "GunSerial.h"

class GunDock : public GunSerial {
   public:
    enum gun_dock_cmd_t {
        CMD_IR_BRIGHTNESS = 0,
        CMD_TOGGLE_TEST_MODE,
        CMD_DOCK_MODE,
        CMD_CALIBRATION_MODE,
        CMD_SAVE_PREFERENCE,
        CMD_CLEAR_EEPROM,

        CMD_EEPROM_READ_USB,
        CMD_EEPROM_READ_TOGGLES,
        CMD_EEPROM_READ_PINS,
        CMD_EEPROM_READ_SETTINGS,
        CMD_EEPROM_READ_PROFILE,

        CMD_EEPROM_UPDATE,
        CMD_TEST_FFB,
        CMD_TEST_BUTTONS,
        CMD_REBOOT
    };

    GunDock(Stream *stream, GunSerialCallback *callback=NULL) {
        _state = STATE_IDLE;
    }

    void process();

   private:
    enum state_t {
        STATE_IDLE,
        STATE_HEADER_START,
        STATE_BRIGHT,
        STATE_MAPPING,
        STATE_MAPPING_DATA,
        STATE_EEPROM,
        STATE_EEPROM_PROFILE,
        STATE_CALIBRATION_IDX,
        STATE_CALIBRATION,
        STATE_TEST_FFB,
        STATE_REBOOT
    };

    uint8_t          _state;
    uint8_t          _buf[100];
    uint8_t          _pos;
};

#endif