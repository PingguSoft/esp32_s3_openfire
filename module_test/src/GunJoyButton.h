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
// mouse button mask
#define MOUSE_LEFT     0x01
#define MOUSE_RIGHT    0x02
#define MOUSE_MIDDLE   0x04
#define MOUSE_BACKWARD 0x08
#define MOUSE_FORWARD  0x10
#define MOUSE_ALL      0x1F

// pad axis
#define PAD_AXIS_X _BV(31)
#define PAD_AXIS_Y _BV(30)
#define PAD_AXIS_Z _BV(29)

// pad button number
#define PAD_BUTTON_A      _BV(0)
#define PAD_BUTTON_B      _BV(1)
#define PAD_BUTTON_C      _BV(2)
#define PAD_BUTTON_X      _BV(3)
#define PAD_BUTTON_Y      _BV(4)
#define PAD_BUTTON_Z      _BV(5)
#define PAD_BUTTON_TL     _BV(6)
#define PAD_BUTTON_TR     _BV(7)
#define PAD_BUTTON_TL2    _BV(8)
#define PAD_BUTTON_TR2    _BV(9)
#define PAD_BUTTON_SELECT _BV(10)
#define PAD_BUTTON_START  _BV(11)
#define PAD_BUTTON_MODE   _BV(12)
#define PAD_BUTTON_THUMBL _BV(13)
#define PAD_BUTTON_THUMBR _BV(14)

#define PAD_BUTTON_SOUTH BUTTON_A
#define PAD_BUTTON_EAST  BUTTON_B
#define PAD_BUTTON_NORTH BUTTON_X
#define PAD_BUTTON_WEST  BUTTON_Y

/// Standard Gamepad HAT/DPAD Buttons (from Linux input event codes)
#define PAD_HAT_CENTER     0
#define PAD_HAT_UP         1
#define PAD_HAT_UP_RIGHT   2
#define PAD_HAT_RIGHT      3
#define PAD_HAT_DOWN_RIGHT 4
#define PAD_HAT_DOWN       5
#define PAD_HAT_DOWN_LEFT  6
#define PAD_HAT_LEFT       7
#define PAD_HAT_UP_LEFT    8

#define PAD_HAT_MASK_X_P   0x01
#define PAD_HAT_MASK_Y_P   0x02
#define PAD_HAT_MASK_X_M   0x04
#define PAD_HAT_MASK_Y_M   0x08

#define GUN_BTN_TRIGGER  PAD_BUTTON_TR
#define GUN_BTN_A        PAD_BUTTON_TL
#define GUN_BTN_B        PAD_BUTTON_Y
#define GUN_BTN_C        PAD_BUTTON_A
#define GUN_BTN_START    PAD_BUTTON_START
#define GUN_BTN_SELECT   PAD_BUTTON_SELECT
#define GUN_BTN_PEDAL    PAD_BUTTON_X

/*
*****************************************************************************************
* GunJoyButton
*****************************************************************************************
*/
class GunJoyButton {
   public:
    typedef struct gun_joy_report {
        int8_t   x;
        int8_t   y;
        uint8_t  hat;
        uint8_t  mouse_buttons;
        uint32_t pad_buttons;

        gun_joy_report() : x(0), y(0), hat(0), mouse_buttons(0), pad_buttons(0) {}

        bool inline operator==(struct gun_joy_report &rep) {
            return (hat == rep.hat && pad_buttons == rep.pad_buttons && mouse_buttons == rep.mouse_buttons);
        }
        bool inline operator!=(struct gun_joy_report &rep) {
            return (hat != rep.hat || pad_buttons != rep.pad_buttons || mouse_buttons != rep.mouse_buttons);
        }
    } report_t;

    GunJoyButton() {
        _state2 = STATE_NONE;
        memset(&_report, 0, sizeof(_report));
        set_auto_trigger(300, 150);
    }

    void set_auto_trigger(uint16_t strt_delay, uint16_t rpt_delay) {
        _auto_strt_delay = strt_delay;
        _auto_rpt_delay  = rpt_delay;
    }
    void      add_button(uint8_t gpio, uint8_t mode, uint16_t mouse_evt, uint32_t pad_evt);
    void      setup(void (*cb)(report_t *report) = NULL, uint16_t strt_delay=0, uint16_t rpt_delay=0);
    bool      loop();
    report_t *get() { return &_report; }
    void      get(report_t *report) { *report = _report; }
    uint8_t   get_hat_mask() { return _hat_mask; }

   private:
    enum { STATE_NONE = 0, STATE_DEBOUNCE, STATE_AUTO_TRG_DELAY, STATE_AUTO_TRG_RPT_DELAY, STATE_WAIT_RELEASE };

    typedef struct pin_sw_info {
        uint8_t  _pin;
        uint8_t  _mode;
        int8_t   _val;
        uint8_t  _state;
        uint16_t _mouse_evt;
        uint32_t _pad_evt;
        pin_sw_info(uint8_t pin, uint8_t mode, uint16_t mouse_evt, uint32_t pad_evt)
            : _pin(pin), _mode(mode), _mouse_evt(mouse_evt), _pad_evt(pad_evt) {}
    } pin_sw_info_t;

    static bool          timer_sw_check_task(pin_sw_info_t *state);
    static bool          timer_sw_pulse_task(pin_sw_info_t *state);
    static bool          timer_adc_check_task(pin_sw_info_t *state);
    static bool          timer_adc_pulse_task(pin_sw_info_t *state);
    static bool          timer_adc_btn_emul_task(pin_sw_info_t *state);
    const static uint8_t _tbl_hat[];
    uint8_t              _state2;

    Timer<20, millis, pin_sw_info_t *> *_timer;
    uint16_t                            _auto_strt_delay;
    uint16_t                            _auto_rpt_delay;
    std::list<pin_sw_info_t *>          _list_sw;
    report_t                            _report;
    uint8_t                             _hat_mask;
    uint8_t                             _hat_mask_rsv;
    void (*_cb)(report_t *report);
};

class ButtonTracker {
   public:
    ButtonTracker(int shift = 0) {
        _shift = shift;
        reset();
    }

    void begin(int btn) {
        _btn       = btn;
        _toggled   = btn ^ _oldBtn;
        _cur_shift = btn & _shift;
    }

    void reset() {
        _btn    = 0;
        _oldBtn = 0;
    }

    void end() { _oldBtn = _btn; }

    void setShift(int shift) { _shift = shift; }

    bool isPressed(int check) {
        int shift = check & _shift;

        if (shift != _cur_shift)
            return false;

        int toggled = _toggled & (~_shift);  // clear shift mask
        return bool((toggled & check) && (_btn & check) == check);
    }

    bool isReleased(int check) {
        int shift = check & _shift;

        if (shift != _cur_shift)
            return false;

        int toggled = _toggled & (~_shift);  // clear shift mask
        return bool((toggled & check) && (_btn & check) == 0);
    }

    bool isToggled(int check) {
        int shift = check & _shift;

        if (shift != _cur_shift)
            return false;

        int toggled = _toggled & (~_shift);  // clear shift mask
        return bool((toggled & check) == check);
    }

    bool isChanged() { return bool(_toggled); }

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