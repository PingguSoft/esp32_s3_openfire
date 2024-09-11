#ifndef __GUN_CALIBRATION_H__
#define __GUN_CALIBRATION_H__

#include <Arduino.h>
#include <arduino-timer.h>

#include "GunCamera.h"
#include "GunHID.h"
#include "GunJoyButton.h"
#include "GunSettings.h"

class GunCalibration {
   public:
    GunCalibration() {}
    void setup(GunSettings *settings, GunHID *hid, GunCamera *cam);
    void begin();
    void end();
    bool loop(uint16_t buttons);

   private:
    void        mouse_ani_begin(int8_t stage);
    bool        is_mouse_move() { return (_ani_info.ctr > 0); }
    static bool timer_mouse_ani_task(GunCalibration *p);

    enum CaliStage_e { Cali_Init = 0, Cali_Top, Cali_Bottom, Cali_Left, Cali_Right, Cali_Center, Cali_Verify };

    struct _ani {
        uint16_t x;
        uint16_t y;
        uint16_t ctr;
        int16_t  dx;
        int16_t  dy;
    };

    GunHID                             *_hid;
    GunCamera                          *_cam;
    GunSettings                        *_settings;
    Timer<1, millis, GunCalibration *> *_timer;
    int8_t                              _stage;
    struct _ani                         _ani_info;
    ButtonTracker                       _btn_trk;
    GunSettings::profile_data_t         _pd_save;
};

#endif