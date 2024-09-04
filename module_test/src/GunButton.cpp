#include "GunButton.h"

#include "config.h"
#include "debug.h"

bool GunButton::pulse_sw(pin_sw_info_t *sw_info) {
    if (sw_info->_on) {
        sw_info->_on = 0;
        (*sw_info->_cb)(sw_info, sw_info->_on);
    }
    return false;
}

bool GunButton::check_sw(pin_sw_info_t *sw_info) {
    GunButton     *p       = (GunButton *)sw_info->_parent;
    int8_t         val     = digitalRead(sw_info->_pin);
    bool           is_rpt  = false;

    if (val == LOW) {
        if (sw_info->_cb && sw_info->_state != STATE_WAIT_RELEASE) {
            sw_info->_on = 1;
            (*sw_info->_cb)((void *)sw_info, sw_info->_on);
            if (p->_auto_trg_rpt_delay > 0)
                p->_timer->in(p->_auto_trg_rpt_delay / 3, pulse_sw, sw_info);
        }

        switch (sw_info->_state) {
            case STATE_DEBOUNCE:
                if (p->_auto_trg_delay > 0) {
                    p->_timer->in(p->_auto_trg_delay, check_sw, sw_info);
                    sw_info->_state = STATE_AUTO_TRG_DELAY;
                } else {
                    sw_info->_state = STATE_WAIT_RELEASE;
                }
                break;

            case STATE_AUTO_TRG_DELAY:
                if (p->_auto_trg_rpt_delay > 0) {
                    p->_timer->every(p->_auto_trg_rpt_delay, check_sw, sw_info);
                    sw_info->_state = STATE_AUTO_TRG_RPT_DELAY;
                } else {
                    sw_info->_state = STATE_WAIT_RELEASE;
                }
                break;

            case STATE_AUTO_TRG_RPT_DELAY:
                is_rpt = true;
                break;
        }
    }

    return is_rpt;
}

void GunButton::check_buttons() {
    int8_t val;

    for (pin_sw_info_t *sw : _list_sw) {
        int8_t val = digitalRead(sw->_pin);

        if (val == HIGH) {
            sw->_state = STATE_NONE;
            if (sw->_on) {
                sw->_on = 0;
                (*sw->_cb)((void *)sw, sw->_on);
            }
        } else {
            if (sw->_state == STATE_NONE) {
                sw->_state = STATE_DEBOUNCE;
                _timer->in(10, check_sw, sw);
            }
        }
        sw++;
    }
}

void GunButton::add_button(uint8_t gpio, uint8_t mode, void (*cb)(void *param, uint8_t state)) {
    for (pin_sw_info_t *sw : _list_sw) {
        if (sw->_pin == gpio) {
            LOGE("%d pin exist\n", gpio);
            return;
        }
    }

    pin_sw_info_t *t = new pin_sw_info_t(gpio, mode, cb);
    _list_sw.push_back(t);
}

void GunButton::setup() {
    LOGV("setup !!!\n");

    for (pin_sw_info_t *sw : _list_sw) {
        pinMode(sw->_pin, sw->_mode);
        sw->_parent = this;
    }
    _timer = new Timer<16, millis, pin_sw_info_t *>();
}

void GunButton::loop() {
    check_buttons();
    _timer->tick();
}
