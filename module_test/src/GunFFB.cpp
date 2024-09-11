#include "GunFFB.h"

#include "config.h"
#include "debug.h"


GunFFB::GunFFB() {
    set_power(200);
    _timer = new Timer<1>();
}

void GunFFB::setup(uint8_t pin, uint16_t hold_delay, Adafruit_NeoPixel *pixel, uint8_t pos) {
    _gpio_pin   = pin;
    _pixel      = pixel;
    _pos        = pos;
    _hold_delay = hold_delay;
    _on         = false;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

bool GunFFB::timer_recoil_task(void *param) {
    GunFFB  *p = (GunFFB *)param;
    uint8_t  pwm;
    uint32_t color;

    if (!p->_on) {
        pwm    = p->_pwm;
        color  = Adafruit_NeoPixel::Color(255, 0, 0);
        p->_on = true;
        p->_timer->in(p->_hold_delay, GunFFB::timer_recoil_task, (void *)p);
    } else {
        pwm    = 0;
        color  = Adafruit_NeoPixel::Color(0, 0, 255);
        p->_on = false;
    }
    analogWrite(p->_gpio_pin, pwm);
    if (p->_pixel) {
        p->_pixel->setPixelColor(0, color);
        p->_pixel->show();
    }

    return false;
}

void GunFFB::trigger() {
    _on = false;
    GunFFB::timer_recoil_task(this);
}

void GunFFB::loop() {
    _timer->tick();
}
