#ifndef __GUN_CAMERA_H__
#define __GUN_CAMERA_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <arduino-timer.h>

#include "DFRobotIRPositionEx.h"
#include "GunSettings.h"
#include "OpenFIRE_Layout.h"
#include "OpenFIRE_Perspective.h"
#include "config.h"
#include "debug.h"

/*
*****************************************************************************************
* GunCamera
*****************************************************************************************
*/
class GunCamera {
   public:
    GunCamera() : _rx(0), _ry(0), _x(0), _y(0), _idx(0), _xAxisArr {0, 0, 0}, _yAxisArr {0, 0, 0} {
        _layout = NULL;
        _perspective = NULL;
    }

    void                  setup(GunSettings *settings);
    void                  update_setting();
    void                  loop();
    OpenFIRE_Layout      *get_layout() { return _layout; }
    OpenFIRE_Perspective *get_perspective() { return _perspective; }
    int                   avail() { return _ir->avail(); }
    int                   rx() { return _rx; }
    int                   ry() { return _ry; }
    int                   x() { return _x; }
    int                   y() { return _y; }

   private:
    void        clk_init(int port, int mclk, uint32_t hz);
    void        update_last_seen();
    static bool timer_camera_task(GunCamera *p);

    Timer<1, millis, GunCamera *>     *_timer;
    DFRobotIRPositionEx               *_ir;
    OpenFIRE_Layout                   *_layout;
    OpenFIRE_Perspective              *_perspective;
    GunSettings                       *_settings;

    int      _rx;
    int      _ry;
    int      _x;
    int      _y;
    int      _xAxisArr[3];
    int      _yAxisArr[3];
    uint8_t  _idx = 0;
    uint16_t _last_seen;
};

#endif