#ifndef __GUN_HID_H__
#define __GUN_HID_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <arduino-timer.h>

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
    virtual void    report_gamepad(int x, int y, uint8_t hat, uint16_t buttons) {}

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
    void    report_gamepad(int x, int y, uint8_t hat, uint16_t buttons) {}
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
    void report_gamepad(int x, int y, uint8_t hat, uint16_t buttons);

    Stream *get_serial();

   private:
};

#endif