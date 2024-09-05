#include "config.h"
#include "debug.h"
#include "GunJoyButton.h"

static GunJoyButton *_parent;

bool GunJoyButton::pulse_sw(pin_sw_info_t *sw_info) {
    if (sw_info->_val) {
        sw_info->_val = 0;
        (*sw_info->_cb)((btn_cb_info_t *)sw_info);
    }
    return false;
}

bool GunJoyButton::check_sw(pin_sw_info_t *sw_info) {
    GunJoyButton *p      = _parent;
    int8_t     val    = digitalRead(sw_info->_pin);
    bool       is_rpt = false;

    if (val == LOW) {
        if (sw_info->_cb && sw_info->_state != STATE_WAIT_RELEASE) {
            sw_info->_val = 1;
            (*sw_info->_cb)((btn_cb_info_t *)sw_info);
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

bool GunJoyButton::check_analog(pin_sw_info_t *sw_info) {
    for (pin_sw_info_t *sw : _parent->_list_sw) {
        if (sw->_mode == ANALOG) {
            uint16_t v = analogRead(sw->_pin);
            sw->_val =  map(v, 0, 4095, -127, 127);
            (*sw->_cb)((btn_cb_info_t *)sw);
        }
    }
    return true;
}

void GunJoyButton::add_button(uint8_t no, uint8_t gpio, uint8_t mode,
                           void (*cb)(btn_cb_info_t *cb_info)) {
    for (pin_sw_info_t *sw_info : _list_sw) {
        if (sw_info->_pin == gpio) {
            LOGE("%d pin exist\n", gpio);
            return;
        }
    }

    pin_sw_info_t *t = new pin_sw_info_t(no, gpio, mode, cb);
    _list_sw.push_back(t);
}

void GunJoyButton::setup(uint16_t auto_trg_delay, uint16_t auto_trg_rpt_delay) {
    LOGV("setup !!!\n");
    _parent = this;
    _timer = new Timer<16, millis, pin_sw_info_t *>();
    _timer->every(20, check_analog, NULL);

    for (pin_sw_info_t *sw_info : _list_sw) {
        pinMode(sw_info->_pin, sw_info->_mode);
    }
    set_auto_trigger(auto_trg_delay, auto_trg_rpt_delay);
}

void GunJoyButton::loop() {
    int8_t val;

    for (pin_sw_info_t *sw_info : _list_sw) {
        if (sw_info->_mode == ANALOG)
            continue;

        int8_t val = digitalRead(sw_info->_pin);
        if (val == HIGH) {
            sw_info->_state = STATE_NONE;
            if (sw_info->_val) {
                sw_info->_val = 0;
                (*sw_info->_cb)((btn_cb_info_t *)sw_info);
            }
        } else {
            if (sw_info->_state == STATE_NONE) {
                sw_info->_state = STATE_DEBOUNCE;
                _timer->in(10, check_sw, sw_info);
            }
        }
    }
    _timer->tick();
}
