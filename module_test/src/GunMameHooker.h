#ifndef __GUN_MAME_HOOKER_H__
#define __GUN_MAME_HOOKER_H__

#include <Arduino.h>

#include "debug.h"

class GunMameHookerCallback {
   public:
    virtual void onMameHookCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) = 0;
};

class GunMameHooker {
   public:
    enum gun_mame_hooker_cmd_t {
        CMD_START = 0,
        CMD_END,
        CMD_MODE,
        CMD_FFB,
    };

    GunMameHooker(Stream *stream) {
        _state    = STATE_IDLE;
        _stream   = stream;
        _callback = NULL;
    }

    Stream *get_stream() { return _stream; }
    void set_callback(GunMameHookerCallback *callback) { _callback = callback; }
    void process();

   private:
    enum state_t {
        STATE_IDLE,
        STATE_CMD_START,
        STATE_CMD_END,
        STATE_CMD_MODE,
        STATE_CMD_MODE_EXT,
        STATE_CMD_FFB,
        STATE_CMD_FFB_PARAM,
        STATE_CMD_FFB_PARAM_DECI,
    };

    GunMameHookerCallback *_callback;
    Stream          *_stream;
    uint8_t          _state;
    uint8_t          _buf[100];
    uint8_t          _pos;
    uint8_t          _dec_pos;
};

#endif