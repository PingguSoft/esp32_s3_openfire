#ifndef __GUN_FFB_H__
#define __GUN_FFB_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <Adafruit_NeoPixel.h>
#include <arduino-timer.h>


/*
*****************************************************************************************
* GunFFB
*****************************************************************************************
*/
class GunFFB {
   public:
    GunFFB() { set_power(200, 50); }

    void setup(uint8_t pin, Adafruit_NeoPixel *pixel = NULL, uint8_t pos = 0);
    void loop();
    void trigger();
    void set_power(uint8_t pwm, uint16_t hold_delay) {
        _pwm        = pwm;
        _hold_delay = hold_delay;
    }

   protected:
    static bool recoil(void *param);

    Timer<2> *_timer;
    uint8_t   _gpio_pin;
    uint8_t   _pwm;
    bool      _on;
    uint16_t  _hold_delay;

    Adafruit_NeoPixel *_pixel;
    uint8_t            _pos;
};
#endif