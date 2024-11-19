#ifndef __GUN_MAME_HOOKER_H__
#define __GUN_MAME_HOOKER_H__

#include <Arduino.h>

#include "debug.h"
#include "GunSerial.h"

class GunMameHooker : public GunSerial {
   public:
    enum gun_mame_hooker_cmd_t {
        CMD_START = 0,
        CMD_END,
        CMD_MODE,
        CMD_FFB,
    };

    GunMameHooker(Stream *stream, GunSerialCallback *callback=NULL) {
        _state = STATE_IDLE;
    }

    void process();

   private:
    enum state_t {
        STATE_IDLE,
        STATE_CMD_START,
        STATE_CMD_END,
        STATE_CMD_MODE,
        STATE_CMD_MODE_EXT_01,
        STATE_CMD_MODE_EXT_D3,
        STATE_CMD_FFB,
        STATE_CMD_FFB_PARAM,
        STATE_CMD_FFB_PARAM_DECI,
    };

    uint8_t          _state;
    uint8_t          _buf[100];
    uint8_t          _pos;
    uint8_t          _dec_pos;
};

#endif