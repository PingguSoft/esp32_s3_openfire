#include "GunJoyButton.h"

#include "config.h"
#include "debug.h"

static GunJoyButton *_parent;

bool GunJoyButton::timer_sw_pulse_task(pin_sw_info_t *sw_info) {
    if (sw_info->_val) {
        sw_info->_val = 0;
        _parent->_report.pad_buttons &= ~sw_info->_pad_evt;
        _parent->_report.mouse_buttons &= ~sw_info->_mouse_evt;
    }
    return false;
}

bool GunJoyButton::timer_sw_check_task(pin_sw_info_t *sw_info) {
    int8_t        val    = digitalRead(sw_info->_pin);
    bool          is_rpt = false;

    if (val == LOW) {
        if (sw_info->_state != STATE_WAIT_RELEASE) {
            sw_info->_val = 1;

            _parent->_report.pad_buttons |= sw_info->_pad_evt;
            _parent->_report.mouse_buttons |= sw_info->_mouse_evt;
            if (_parent->_auto_rpt_delay > 0)
                _parent->_timer->in(_parent->_auto_rpt_delay / 2, timer_sw_pulse_task, sw_info);
        }

        switch (sw_info->_state) {
            case STATE_DEBOUNCE:
                if (_parent->_auto_strt_delay > 0) {
                    _parent->_timer->in(_parent->_auto_strt_delay, timer_sw_check_task, sw_info);
                    sw_info->_state = STATE_AUTO_TRG_DELAY;
                } else {
                    sw_info->_state = STATE_WAIT_RELEASE;
                }
                break;

            case STATE_AUTO_TRG_DELAY:
                if (_parent->_auto_rpt_delay > 0) {
                    _parent->_timer->every(_parent->_auto_rpt_delay, timer_sw_check_task, sw_info);
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

const uint8_t GunJoyButton::_tbl_hat[] = {
    0,
    PAD_HAT_RIGHT,
    PAD_HAT_UP,
    PAD_HAT_UP_RIGHT,
    PAD_HAT_LEFT,
    0,
    PAD_HAT_UP_LEFT,
    0,
    PAD_HAT_DOWN,
    PAD_HAT_DOWN_RIGHT,
    0,
    0,
    PAD_HAT_DOWN_LEFT,
    0,
    0,
    0
};

bool GunJoyButton::timer_adc_pulse_task(pin_sw_info_t *sw_info) {
    _parent->_hat_mask = 0;
    _parent->_report.hat = _tbl_hat[_parent->_hat_mask & 0x0f];
    return false;
}

bool GunJoyButton::timer_adc_btn_emul_task(pin_sw_info_t *sw_info) {
    bool is_rpt = false;

    if (_parent->_state2 == STATE_NONE)
        return false;

    if (_parent->_state2 != STATE_WAIT_RELEASE) {
        _parent->_hat_mask = _parent->_hat_mask_rsv;
        _parent->_report.hat = _tbl_hat[_parent->_hat_mask & 0x0f];
        if (_parent->_auto_rpt_delay > 0)
            _parent->_timer->in(_parent->_auto_rpt_delay / 2, timer_adc_pulse_task, sw_info);
    }

    switch (_parent->_state2) {
        case STATE_DEBOUNCE:
            if (_parent->_auto_strt_delay > 0) {
                _parent->_timer->in(_parent->_auto_strt_delay, timer_adc_btn_emul_task, sw_info);
                _parent->_state2 = STATE_AUTO_TRG_DELAY;
            } else {
                _parent->_state2 = STATE_NONE;
            }
            break;

        case STATE_AUTO_TRG_DELAY:
            if (_parent->_auto_rpt_delay > 0) {
                _parent->_timer->every(_parent->_auto_rpt_delay, timer_adc_btn_emul_task, sw_info);
                _parent->_state2 = STATE_AUTO_TRG_RPT_DELAY;
            } else {
                _parent->_state2 = STATE_WAIT_RELEASE;
            }
            break;

        case STATE_AUTO_TRG_RPT_DELAY:
            is_rpt = true;
            break;
    }
    return is_rpt;
}

bool GunJoyButton::timer_adc_check_task(pin_sw_info_t *sw_info) {
    uint16_t v, min_adc, max_adc;
    uint8_t mask = 0;

    for (pin_sw_info_t *sw : _parent->_list_sw) {
        if (sw->_mode == ANALOG) {
            v = analogRead(sw->_pin);
            min_adc = (sw->_pad_evt >>  0) & 0xFFF;
            max_adc = (sw->_pad_evt >> 12) & 0xFFF;
            sw->_val = map(constrain(v, min_adc, max_adc), min_adc, max_adc, -127, 127);

            // LOGV("%2d, %5d, %5d\n", sw->_pin, v, sw->_val);
            if (sw->_pad_evt & PAD_AXIS_X) {
                _parent->_report.x = sw->_val;
                if (sw->_val > 64)
                    mask |= PAD_HAT_MASK_X_P;
                else if (sw->_val < -64)
                    mask |= PAD_HAT_MASK_X_M;
            }
            else if (sw->_pad_evt & PAD_AXIS_Y) {
                _parent->_report.y = sw->_val;
                if (sw->_val > 64)
                    mask |= PAD_HAT_MASK_Y_P;
                else if (sw->_val < -64)
                    mask |= PAD_HAT_MASK_Y_M;
            }
        }
    }
    _parent->_hat_mask_rsv = mask;

    if (!mask) {
        _parent->_state2 = STATE_NONE;
        _parent->_hat_mask = 0;
        _parent->_report.hat = _tbl_hat[_parent->_hat_mask & 0x0f];
    } else if (_parent->_state2 == STATE_NONE) {
        _parent->_state2 = STATE_DEBOUNCE;
        _parent->_timer->in(0, timer_adc_btn_emul_task, sw_info);
    }

    return true;
}

void GunJoyButton::add_button(uint8_t gpio, uint8_t mode, uint16_t mouse_evt, uint32_t pad_evt) {
    for (pin_sw_info_t *sw_info : _list_sw) {
        if (sw_info->_pin == gpio) {
            LOGE("%d pin exist\n", gpio);
            return;
        }
    }
    pin_sw_info_t *t = new pin_sw_info_t(gpio, mode, mouse_evt, pad_evt);
    _list_sw.push_back(t);
}

void GunJoyButton::setup(void (*cb)(report_t *report), uint16_t auto_trg_delay, uint16_t auto_trg_rpt_delay) {
    LOGV("setup !!!\n");
    _cb     = cb;
    _parent = this;
    _timer  = new Timer<20, millis, pin_sw_info_t *>();

    for (pin_sw_info_t *sw_info : _list_sw) {
        pinMode(sw_info->_pin, sw_info->_mode);
    }
    set_auto_trigger(auto_trg_delay, auto_trg_rpt_delay);

    _timer->every(20, timer_adc_check_task, NULL);
}

bool GunJoyButton::loop() {
    report_t prev = _report;

    for (pin_sw_info_t *sw_info : _list_sw) {
        if (sw_info->_mode == ANALOG)
            continue;

        int8_t val = digitalRead(sw_info->_pin);
        if (val == HIGH) {
            sw_info->_state = STATE_NONE;
            if (sw_info->_val) {
                sw_info->_val = 0;
                _report.pad_buttons &= ~sw_info->_pad_evt;
                _report.mouse_buttons &= ~sw_info->_mouse_evt;
            }
        } else {
            if (sw_info->_state == STATE_NONE) {
                sw_info->_state = STATE_DEBOUNCE;
                _timer->in(10, timer_sw_check_task, sw_info);
            }
        }
    }

    _timer->tick();

    if (_report != prev) {
        if (_cb)
            (*_cb)(&_report);
        return true;
    }
    return false;
}
