#include "GunJoyButton.h"

#include "config.h"
#include "debug.h"

static GunJoyButton *_parent;

bool GunJoyButton::timer_sw_pulse_task(pin_sw_info_t *sw_info) {
    if (sw_info->_val) {
        sw_info->_val = 0;
        _parent->_buttons &= ~(1 << sw_info->_no);
    }
    return false;
}

bool GunJoyButton::timer_sw_check_task(pin_sw_info_t *sw_info) {
    GunJoyButton *p      = _parent;
    int8_t        val    = digitalRead(sw_info->_pin);
    bool          is_rpt = false;

    if (val == LOW) {
        if (sw_info->_state != STATE_WAIT_RELEASE) {
            sw_info->_val = 1;

            p->_buttons |= (1 << sw_info->_no);
            if (p->_auto_trg_rpt_delay > 0)
                p->_timer->in(p->_auto_trg_rpt_delay / 3, timer_sw_pulse_task, sw_info);
        }

        switch (sw_info->_state) {
            case STATE_DEBOUNCE:
                if (p->_auto_trg_delay > 0) {
                    p->_timer->in(p->_auto_trg_delay, timer_sw_check_task, sw_info);
                    sw_info->_state = STATE_AUTO_TRG_DELAY;
                } else {
                    sw_info->_state = STATE_WAIT_RELEASE;
                }
                break;

            case STATE_AUTO_TRG_DELAY:
                if (p->_auto_trg_rpt_delay > 0) {
                    p->_timer->every(p->_auto_trg_rpt_delay, timer_sw_check_task, sw_info);
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

bool GunJoyButton::timer_adc_check_task(pin_sw_info_t *sw_info) {
    for (pin_sw_info_t *sw : _parent->_list_sw) {
        if (sw->_mode == ANALOG) {
            uint16_t v = analogRead(sw->_pin);
            sw->_val   = map(v, 0, 4095, -127, 127);

            if (sw->_pin == PIN_JOY_ADC_X)
                _parent->_x = sw->_val;
            else if (sw->_pin == PIN_JOY_ADC_Y)
                _parent->_y = sw->_val;
        }
    }
    return true;
}

void GunJoyButton::add_button(uint8_t no, uint8_t gpio, uint8_t mode) {
    for (pin_sw_info_t *sw_info : _list_sw) {
        if (sw_info->_pin == gpio) {
            LOGE("%d pin exist\n", gpio);
            return;
        }
    }
    pin_sw_info_t *t = new pin_sw_info_t(no, gpio, mode);
    _list_sw.push_back(t);
}

void GunJoyButton::setup(void (*cb)(int8_t x, int8_t y, uint8_t buttons), uint16_t auto_trg_delay,
                         uint16_t auto_trg_rpt_delay) {
    LOGV("setup !!!\n");
    _cb     = cb;
    _parent = this;
    _timer  = new Timer<16, millis, pin_sw_info_t *>();

    for (pin_sw_info_t *sw_info : _list_sw) {
        pinMode(sw_info->_pin, sw_info->_mode);
    }
    set_auto_trigger(auto_trg_delay, auto_trg_rpt_delay);

    _timer->every(20, timer_adc_check_task, NULL);
}

bool GunJoyButton::loop() {
    int8_t  x        = _x;
    int8_t  y        = _y;
    uint8_t old_btns = _buttons;

    for (pin_sw_info_t *sw_info : _list_sw) {
        if (sw_info->_mode == ANALOG)
            continue;

        int8_t val = digitalRead(sw_info->_pin);
        if (val == HIGH) {
            sw_info->_state = STATE_NONE;
            if (sw_info->_val) {
                sw_info->_val = 0;
                _buttons &= ~(1 << sw_info->_no);
            }
        } else {
            if (sw_info->_state == STATE_NONE) {
                sw_info->_state = STATE_DEBOUNCE;
                _timer->in(10, timer_sw_check_task, sw_info);
            }
        }
    }

    _timer->tick();

    if (x != _x || y != y || _buttons != old_btns) {
        if (_cb)
            (*_cb)(_x, _y, _buttons);
        return true;
    }
    return false;
}
