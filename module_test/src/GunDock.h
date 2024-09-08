#include <Arduino.h>

#include "GunSettings.h"
#include "debug.h"


class GunDockCallback {
   public:
    virtual void onCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) = 0;
};

class GunDock {
   public:
    enum gun_dock_cmd_t {
        CMD_IR_BRIGHTNESS = 0,
        CMD_DOCK_MODE,
        CMD_DOCK_MODE_EXIT,
        CMD_CAL_MODE,
        CMD_SAVE_PREFERENCE,
        CMD_CLEAR_EEPROM,

        CMD_EEPROM_READ_USB,
        CMD_EEPROM_READ_TOGGLES,
        CMD_EEPROM_READ_PINS,
        CMD_EEPROM_READ_SETTINGS,
        CMD_EEPROM_READ_PROFILE,

        CMD_EEPROM_UPDATE_USB,
        CMD_EEPROM_UPDATE_TOGGLES,
        CMD_EEPROM_UPDATE_PINS,
        CMD_EEPROM_UPDATE_SETTINGS,
        CMD_EEPROM_UPDATE_PROFILE,
    };

    GunDock(GunSettings *settings, Stream *stream) {
        _state    = STATE_IDLE;
        _settings = settings;
        _stream   = stream;
        _callback = NULL;
    }

    void set_callback(GunDockCallback *callback) { _callback = callback; }
    void process();

   private:
    enum state_t {
        STATE_IDLE,
        STATE_HEADER_START,
        STATE_BRIGHT,
        STATE_EEPROM,
        STATE_EEPROM_PROFILE,
    };

    GunDockCallback *_callback;
    GunSettings     *_settings;
    Stream          *_stream;
    uint8_t          _state;
};