#include "GunFFB.h"

#include "config.h"
#include "debug.h"


void GunFFB::setup(uint8_t pin, Adafruit_NeoPixel *pixel, uint8_t pos) {
    _gpio_pin = pin;
    set_power(200, 50);

    _pixel = pixel;
    _pos   = pos;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    _on    = false;
    _timer = new Timer<2>();
}

bool GunFFB::recoil(void *param) {
    GunFFB  *p = (GunFFB *)param;
    uint8_t  pwm;
    uint32_t color;

    if (!p->_on) {
        LOGV("BANG !!!\n");
        pwm    = p->_pwm;
        color  = Adafruit_NeoPixel::Color(255, 0, 0);
        p->_on = true;
        p->_timer->in(p->_hold_delay, GunFFB::recoil, (void *)p);
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
    GunFFB::recoil(this);
}

void GunFFB::loop() {
    _timer->tick();
}
