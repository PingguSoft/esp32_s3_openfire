#ifndef __GUN_SERIAL_H__
#define __GUN_SERIAL_H__

#include <Arduino.h>

#include "debug.h"

class GunSerialCallback {
   public:
    virtual void onSerialCallback(uint8_t cmd, uint8_t *pData, uint16_t size, Stream *stream) = 0;
};

class GunSerial {
   public:
    GunSerial() { _stream = NULL; }
    GunSerial(Stream *stream, GunSerialCallback *callback=NULL) {
        _stream = stream;
        _callback = callback;
    }
    void    set_callback(GunSerialCallback *callback) { _callback = callback; }
    Stream *get_stream() { return _stream; }

    virtual void process() = 0;

   protected:
    GunSerialCallback *_callback;
    Stream            *_stream;
};

#endif