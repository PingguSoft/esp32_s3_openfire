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
    GunFFB() { set_power(200); }

    void setup(uint8_t pin, uint16_t hold_delay, Adafruit_NeoPixel *pixel = NULL, uint8_t pos = 0);
    void loop();
    void trigger();
    void set_power(uint8_t pwm) { _pwm = pwm; }

   protected:
    static bool timer_recoil_task(void *param);

    Timer<1> *_timer;
    uint8_t   _gpio_pin;
    uint8_t   _pwm;
    bool      _on;
    uint16_t  _hold_delay;

    Adafruit_NeoPixel *_pixel;
    uint8_t            _pos;
};
#endif