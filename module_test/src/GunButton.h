#ifndef __GUN_BUTTON_H__
#define __GUN_BUTTON_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <arduino-timer.h>

#include <list>


/*
*****************************************************************************************
* GunButton
*****************************************************************************************
*/
class GunButton {
   public:
    GunButton() { set_auto_trigger(300, 150); }

    void set_auto_trigger(uint16_t auto_trg_delay = 0, uint16_t auto_trg_rpt_delay = 0) {
        _auto_trg_delay     = auto_trg_delay;
        _auto_trg_rpt_delay = auto_trg_rpt_delay;
    }
    void add_button(uint8_t gpio, uint8_t mode, void (*cb)(void *param, uint8_t state) = NULL);
    void setup();
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
        uint8_t _pin;
        uint8_t _mode;
        uint8_t _on;
        uint8_t _state;
        void   *_parent;
        void (*_cb)(void *param, uint8_t state);
        pin_sw_info(uint8_t pin, uint8_t mode, void (*cb)(void *param, uint8_t state))
            : _pin(pin), _mode(mode), _cb(cb) {}
    } pin_sw_info_t;

    static bool check_sw(pin_sw_info_t *state);
    static bool pulse_sw(pin_sw_info_t *state);
    void        check_buttons();

    Timer<16, millis, pin_sw_info_t *> *_timer;
    uint16_t   _auto_trg_delay;
    uint16_t   _auto_trg_rpt_delay;

    std::list<pin_sw_info_t *> _list_sw;
};

#endif