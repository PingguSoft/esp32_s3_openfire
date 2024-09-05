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
* GunJoyButton
*****************************************************************************************
*/
class GunJoyButton {
   public:
    typedef struct {
        uint8_t no;
        uint8_t pin;
        uint8_t mode;
        int8_t  val;
    } btn_cb_info_t;

    GunJoyButton() { set_auto_trigger(300, 150); }

    void set_auto_trigger(uint16_t auto_trg_delay, uint16_t auto_trg_rpt_delay) {
        _auto_trg_delay     = auto_trg_delay;
        _auto_trg_rpt_delay = auto_trg_rpt_delay;
    }
    void add_button(uint8_t no, uint8_t gpio, uint8_t mode,
                    void (*cb)(btn_cb_info_t *cb_info) = NULL);
    void setup(uint16_t auto_trg_delay = 0, uint16_t auto_trg_rpt_delay = 0);
    void loop();

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
        void (*_cb)(btn_cb_info_t *cb_info);
        pin_sw_info(uint8_t no, uint8_t pin, uint8_t mode, void (*cb)(btn_cb_info_t *cb_info))
            : _no(no), _pin(pin), _mode(mode), _cb(cb) {}
    } pin_sw_info_t;

    static bool check_sw(pin_sw_info_t *state);
    static bool pulse_sw(pin_sw_info_t *state);
    static bool check_analog(pin_sw_info_t *state);

    Timer<16, millis, pin_sw_info_t *> *_timer;
    uint16_t                            _auto_trg_delay;
    uint16_t                            _auto_trg_rpt_delay;
    std::list<pin_sw_info_t *>          _list_sw;
};

#endif