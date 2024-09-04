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
    virtual void setup() {}
    virtual void loop() {}
    virtual Stream *get_serial() { return &Serial0; }
    void report_mouse(int x, int y) {}
    void report_gamepad(int x, int y, uint8_t hat, uint8_t buttons) {}

   protected:
    std::string _devMfr;
    std::string _devName;
    uint16_t    _vid;
    uint16_t    _pid;
    Timer<2>   *_timer;
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
    void setup();
    void loop();
    void report_mouse(int x, int y);
    void report_gamepad(int x, int y, uint8_t hat, uint8_t buttons);
    Stream *get_serial() { return &Serial0; }

   private:
    static bool test(void *param);

    void *_tmr_handle = NULL;
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
    void loop();
    void report_mouse(int x, int y);
    void report_gamepad(int x, int y, uint8_t hat, uint8_t buttons);

    Stream *get_serial();

   private:
    static bool test(void *param);

    void *_tmr_handle = NULL;
};

#endif