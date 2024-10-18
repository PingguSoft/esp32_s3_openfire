#include "GunJoyButton.h"

#include "config.h"
#include "debug.h"

// static GunJoyButton *_parent;

GunJoyButton::GunJoyButton() {
    _emul_state = STATE_NONE;
    memset(&_report, 0, sizeof(_report));
    set_auto_trigger(300, 150);
}

bool GunJoyButton::timer_sw_pulse_task(void *param) {
    pin_sw_info_t *info = (pin_sw_info_t *)param;
    if (info->_val) {
        info->_val = 0;
        info->_parent->_report.pad_buttons &= ~info->_pad_evt;
        info->_parent->_report.mouse_buttons &= ~info->_mouse_evt;
    }
    return false;
}

bool GunJoyButton::timer_sw_check_task(void *param) {
    pin_sw_info_t *info   = (pin_sw_info_t *)param;
    int8_t         val    = digitalRead(info->_pin);
    bool           is_rpt = false;

    if (val == LOW) {
        if (info->_state != STATE_WAIT_RELEASE) {
            info->_val = 1;

            info->_parent->_report.pad_buttons |= info->_pad_evt;
            info->_parent->_report.mouse_buttons |= info->_mouse_evt;
            if (info->_parent->_auto_rpt_delay > 0)
                info->_parent->_timer->in(info->_parent->_auto_rpt_delay / 2, timer_sw_pulse_task, info);
        }

        switch (info->_state) {
            case STATE_DEBOUNCE:
                if (info->_parent->_auto_strt_delay > 0) {
                    info->_parent->_timer->in(info->_parent->_auto_strt_delay, timer_sw_check_task, info);
                    info->_state = STATE_AUTO_TRG_DELAY;
                } else {
                    info->_state = STATE_WAIT_RELEASE;
                }
                break;

            case STATE_AUTO_TRG_DELAY:
                if (info->_parent->_auto_rpt_delay > 0) {
                    info->_parent->_timer->every(info->_parent->_auto_rpt_delay, timer_sw_check_task, info);
                    info->_state = STATE_AUTO_TRG_RPT_DELAY;
                } else {
                    info->_state = STATE_WAIT_RELEASE;
                }
                break;

            case STATE_AUTO_TRG_RPT_DELAY:
                is_rpt = true;
                break;
        }
    }

    return is_rpt;
}

// clang-format off
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
// clang-format on

bool GunJoyButton::timer_adc_pulse_task(void *param) {
    GunJoyButton *parent = (GunJoyButton *)param;
    parent->_hat_mask    = 0;
    parent->_report.hat  = _tbl_hat[parent->_hat_mask & 0x0f];
    return false;
}

bool GunJoyButton::timer_adc_btn_emul_task(void *param) {
    GunJoyButton *parent = (GunJoyButton *)param;
    bool          is_rpt = false;

    if (parent->_emul_state == STATE_NONE)
        return false;

    if (parent->_emul_state != STATE_WAIT_RELEASE) {
        parent->_hat_mask   = parent->_hat_mask_rsv;
        parent->_report.hat = _tbl_hat[parent->_hat_mask & 0x0f];
        if (parent->_auto_rpt_delay > 0)
            parent->_timer->in(parent->_auto_rpt_delay / 2, timer_adc_pulse_task, param);
    }

    switch (parent->_emul_state) {
        case STATE_DEBOUNCE:
            if (parent->_auto_strt_delay > 0) {
                parent->_timer->in(parent->_auto_strt_delay, timer_adc_btn_emul_task, param);
                parent->_emul_state = STATE_AUTO_TRG_DELAY;
            } else {
                parent->_emul_state = STATE_NONE;
            }
            break;

        case STATE_AUTO_TRG_DELAY:
            if (parent->_auto_rpt_delay > 0) {
                parent->_timer->every(parent->_auto_rpt_delay, timer_adc_btn_emul_task, param);
                parent->_emul_state = STATE_AUTO_TRG_RPT_DELAY;
            } else {
                parent->_emul_state = STATE_WAIT_RELEASE;
            }
            break;

        case STATE_AUTO_TRG_RPT_DELAY:
            is_rpt = true;
            break;
    }
    return is_rpt;
}

bool GunJoyButton::timer_adc_check_task(void *param) {
    GunJoyButton *parent = (GunJoyButton *)param;
    uint16_t      v, min_adc, max_adc;
    uint8_t       mask = 0;

    for (pin_sw_info_t *sw : parent->_list_sw) {
        if (sw->_mode == ANALOG) {
            v        = analogRead(sw->_pin);
            min_adc  = (sw->_pad_evt >> 0) & 0xFFF;
            max_adc  = (sw->_pad_evt >> 12) & 0xFFF;
            sw->_val = map(constrain(v, min_adc, max_adc), min_adc, max_adc, -127, 127);

            // LOGV("%2d, %5d, %5d\n", sw->_pin, v, sw->_val);
            if (sw->_pad_evt & PAD_AXIS_X) {
                parent->_report.x = sw->_val;
                if (sw->_val > 64)
                    mask |= PAD_HAT_MASK_X_P;
                else if (sw->_val < -64)
                    mask |= PAD_HAT_MASK_X_M;
            } else if (sw->_pad_evt & PAD_AXIS_Y) {
                parent->_report.y = sw->_val;
                if (sw->_val > 64)
                    mask |= PAD_HAT_MASK_Y_P;
                else if (sw->_val < -64)
                    mask |= PAD_HAT_MASK_Y_M;
            }
        }
    }
    parent->_hat_mask_rsv = mask;

    if (!mask) {
        parent->_emul_state = STATE_NONE;
        parent->_hat_mask   = 0;
        parent->_report.hat = _tbl_hat[parent->_hat_mask & 0x0f];
    } else if (parent->_emul_state == STATE_NONE) {
        parent->_emul_state = STATE_DEBOUNCE;
        parent->_timer->in(0, timer_adc_btn_emul_task, param);
    }

    return true;
}

void GunJoyButton::add_button(uint8_t gpio, uint8_t mode, uint16_t mouse_evt, uint32_t pad_evt) {
    for (pin_sw_info_t *info : _list_sw) {
        if (info->_pin == gpio) {
            LOGE("%d pin exist\n", gpio);
            return;
        }
    }
    pin_sw_info_t *t = new pin_sw_info_t(this, gpio, mode, mouse_evt, pad_evt);
    _list_sw.push_back(t);
}

void GunJoyButton::setup(void (*cb)(report_t *report), uint16_t auto_trg_delay, uint16_t auto_trg_rpt_delay) {
    LOGV("setup !!!\n");
    _cb    = cb;
    _timer = new Timer<16, millis, void *>();

    for (pin_sw_info_t *info : _list_sw) {
        pinMode(info->_pin, info->_mode);
    }
    set_auto_trigger(auto_trg_delay, auto_trg_rpt_delay);

    _timer->every(20, timer_adc_check_task, this);
}

bool GunJoyButton::loop() {
    report_t prev = _report;

    for (pin_sw_info_t *info : _list_sw) {
        if (info->_mode == ANALOG)
            continue;

        int8_t val = digitalRead(info->_pin);
        if (val == HIGH) {
            info->_state = STATE_NONE;
            if (info->_val) {
                info->_val = 0;
                _report.pad_buttons &= ~info->_pad_evt;
                _report.mouse_buttons &= ~info->_mouse_evt;
            }
        } else {
            if (info->_state == STATE_NONE) {
                info->_state = STATE_DEBOUNCE;
                _timer->in(10, timer_sw_check_task, info);
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
