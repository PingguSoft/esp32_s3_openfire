#ifndef __GUN_DOCK_H__
#define __GUN_DOCK_H__

#include <Arduino.h>

#include "debug.h"

class GunDockCallback {
   public:
    virtual void onDockCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) = 0;
};

class GunDock {
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

    GunDock(Stream *stream) {
        _state    = STATE_IDLE;
        _stream   = stream;
        _callback = NULL;
    }

    Stream *get_stream() { return _stream; }
    void set_callback(GunDockCallback *callback) { _callback = callback; }
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

    GunDockCallback *_callback;
    Stream          *_stream;
    uint8_t          _state;
    uint8_t          _buf[100];
    uint8_t          _pos;
};

#endif