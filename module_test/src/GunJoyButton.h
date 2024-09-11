#ifndef __GUN_JOY_BUTTON_H__
#define __GUN_JOY_BUTTON_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <arduino-timer.h>
#include <list>

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/


/*
*****************************************************************************************
* GunJoyButton
*****************************************************************************************
*/
class GunJoyButton {
   public:
    GunJoyButton() : _x(0), _y(0), _buttons(0) { set_auto_trigger(300, 150); }

    void set_auto_trigger(uint16_t auto_trg_delay, uint16_t auto_trg_rpt_delay) {
        _auto_trg_delay     = auto_trg_delay;
        _auto_trg_rpt_delay = auto_trg_rpt_delay;
    }
    void add_button(uint8_t no, uint8_t gpio, uint8_t mode);
    void setup(void (*cb)(int8_t x, int8_t y, uint8_t buttons)=NULL, uint16_t auto_trg_delay = 0,
               uint16_t auto_trg_rpt_delay = 0);
    bool loop();

    void get(int8_t *x, int8_t *y, uint8_t *buttons) {
        *x       = _x;
        *y       = _y;
        *buttons = _buttons;
    }

   private:
    enum {
        STATE_NONE = 0,
        STATE_DEBOUNCE,
        STATE_AUTO_TRG_DELAY,
        STATE_AUTO_TRG_RPT_DELAY,
        STATE_WAIT_RELEASE
    };

    typedef struct pin_sw_info {
        uint8_t _no;
        uint8_t _pin;
        uint8_t _mode;
        int8_t  _val;
        uint8_t _state;
        pin_sw_info(uint8_t no, uint8_t pin, uint8_t mode) : _no(no), _pin(pin), _mode(mode) {}
    } pin_sw_info_t;

    static bool timer_sw_check_task(pin_sw_info_t *state);
    static bool timer_sw_pulse_task(pin_sw_info_t *state);
    static bool timer_adc_check_task(pin_sw_info_t *state);

    Timer<16, millis, pin_sw_info_t *> *_timer;
    uint16_t                            _auto_trg_delay;
    uint16_t                            _auto_trg_rpt_delay;
    std::list<pin_sw_info_t *>          _list_sw;
    int8_t                              _x, _y;
    uint8_t                             _buttons;
    void (*_cb)(int8_t x, int8_t y, uint8_t buttons);
};

class ButtonTracker {
   public:
    ButtonTracker(int shift = 0) {
        _shift  = shift;
        _btn    = 0;
        _oldBtn = 0;
    }

    void begin(int btn) {
        _btn       = btn;
        _toggled   = btn ^ _oldBtn;
        _cur_shift = btn & _shift;
    }

    void end() { _oldBtn = _btn; }

    void setShift(int shift) { _shift = shift; }

    bool isPressed(int check) {
        int shift = check & _shift;

        if (shift != _cur_shift)
            return false;

        int toggled = _toggled & (~_shift);  // clear shift mask
        return bool((toggled & check) && (_btn & check));
    }

    bool isReleased(int check) {
        int shift = check & _shift;

        if (shift != _cur_shift)
            return false;

        int toggled = _toggled & (~_shift);  // clear shift mask
        return bool((toggled & check) && !(_btn & check));
    }

    bool isToggled(int check) {
        int shift = check & _shift;

        if (shift != _cur_shift)
            return false;

        int toggled = _toggled & (~_shift);  // clear shift mask
        return bool(toggled & check);
    }

    bool isToggled() { return bool(_toggled); }

    bool isOn(int check) { return bool(_btn & check); }

    bool isOff(int check) { return bool(!(_btn & check)); }

   private:
    int _shift;
    int _cur_shift;
    int _btn;
    int _toggled;
    int _oldBtn;
};

#endif