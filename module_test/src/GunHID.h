#ifndef __GUN_HID_H__
#define __GUN_HID_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <arduino-timer.h>

// mouse button mask
#define MOUSE_LEFT     0x01
#define MOUSE_RIGHT    0x02
#define MOUSE_MIDDLE   0x04
#define MOUSE_BACKWARD 0x08
#define MOUSE_FORWARD  0x10
#define MOUSE_ALL      0x1F

// pad button number
#define PAD_BUTTON_A      0
#define PAD_BUTTON_B      1
#define PAD_BUTTON_C      2
#define PAD_BUTTON_X      3
#define PAD_BUTTON_Y      4
#define PAD_BUTTON_Z      5
#define PAD_BUTTON_TL     6
#define PAD_BUTTON_TR     7
#define PAD_BUTTON_TL2    8
#define PAD_BUTTON_TR2    9
#define PAD_BUTTON_SELECT 10
#define PAD_BUTTON_START  11
#define PAD_BUTTON_MODE   12
#define PAD_BUTTON_THUMBL 13
#define PAD_BUTTON_THUMBR 14

#define PAD_BUTTON_SOUTH BUTTON_A
#define PAD_BUTTON_EAST  BUTTON_B
#define PAD_BUTTON_NORTH BUTTON_X
#define PAD_BUTTON_WEST  BUTTON_Y

/// Standard Gamepad HAT/DPAD Buttons (from Linux input event codes)
#define PAS_HAT_CENTER     0
#define PAS_HAT_UP         1
#define PAS_HAT_UP_RIGHT   2
#define PAS_HAT_RIGHT      3
#define PAS_HAT_DOWN_RIGHT 4
#define PAS_HAT_DOWN       5
#define PAS_HAT_DOWN_LEFT  6
#define PAS_HAT_LEFT       7
#define PAS_HAT_UP_LEFT    8

/*
*****************************************************************************************
* GunHID
*****************************************************************************************
*/
class GunHID {
   public:
    GunHID(std::string devMfr, std::string devName, uint16_t vid, uint16_t pid) {
        _devMfr  = devMfr;
        _devName = devName;
        _vid     = vid;
        _pid     = pid;
    }
    virtual void    setup() {}
    virtual Stream *get_serial() { return &Serial0; }
    virtual void    report_mouse(int x, int y, uint8_t buttons) {}
    virtual void    report_gamepad(int x, int y, uint8_t hat, uint8_t buttons) {}

    int get_mouse_x() { return _mouse_x; }
    int get_mouse_y() { return _mouse_y; }

    static const uint16_t mouse_max_x = 32767;
    static const uint16_t mouse_max_y = 32767;


   protected:
    std::string _devMfr;
    std::string _devName;
    uint16_t    _vid;
    uint16_t    _pid;
    int         _mouse_x;
    int         _mouse_y;
    uint8_t     _mouse_buttons;
};

/*
*****************************************************************************************
* GunHIDBLE
*****************************************************************************************
*/
class GunHIDBLE : public GunHID {
   public:
    GunHIDBLE(std::string devMfr, std::string devName, uint16_t vid = 0, uint16_t pid = 0)
        : GunHID(devMfr, devName, vid, pid) {}
    void    setup();
    void    report_mouse(int x, int y, uint8_t buttons);
    void    report_gamepad(int x, int y, uint8_t hat, uint8_t buttons) {}
    Stream *get_serial() { return &Serial0; }

   private:
};

/*
*****************************************************************************************
* GunHIDUSB
*****************************************************************************************
*/
class GunHIDUSB : public GunHID {
   public:
    GunHIDUSB(std::string devMfr, std::string devName, uint16_t vid, uint16_t pid)
        : GunHID(devMfr, devName, vid, pid) {}
    void setup();
    void report_mouse(int x, int y, uint8_t buttons);
    void report_gamepad(int x, int y, uint8_t hat, uint8_t buttons);

    Stream *get_serial();

   private:
};

#endif