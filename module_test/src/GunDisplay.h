#ifndef __GUN_DISPLAY_H__
#define __GUN_DISPLAY_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <Wire.h>
#include <arduino-timer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <vector>
#include "config.h"
#include "debug.h"

/*
*****************************************************************************************
* GunDisplay
*****************************************************************************************
*/
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

#define unpack_uint16(x) ((x >> 0) & 0xff), ((x >> 8) & 0xff)
#define bitmap_width(x)  (x[0] | ((uint16_t)x[1] << 8))
#define bitmap_height(x) (x[2] | ((uint16_t)x[3] << 8))

class GunDisplay {
   public:
    GunDisplay() {}
    bool setup(TwoWire *wire);

    void draw_ir(int pointX[4], int pointY[4]);
    void draw_ammo(uint8_t ammo);
    void draw_life(uint8_t life);

    void draw_centered_text(char *text, uint8_t flag = 0);
    void tokenize(char *line, char *token, std::vector<char *> &tokens);

    void draw_bitmap(int16_t x, int16_t y, const uint8_t bitmap[], uint16_t color);

    Adafruit_SSD1306 *drv()  { return _disp_drv; }

    enum ScreenSerialInit_e { ScreenSerial_None = 0, ScreenSerial_Life, ScreenSerial_Ammo, ScreenSerial_Both };

    /// @brief Whether life updates are in lifebar or life glyphs form
    bool lifeBar = false;

    /// @brief Which layout we use for serial mode
    uint8_t serialDisplayType = 0;

   private:
    Adafruit_SSD1306 *_disp_drv;
    bool ammoEmpty = false;
    bool lifeEmpty = false;

    uint8_t currentAmmo;
    uint8_t currentLife;

    // timestamps, in case we need them for periodic tasks in IdleOps()
    unsigned long ammoTimestamp = 0;
    unsigned long lifeTimestamp = 0;
    unsigned long idleTimeStamp = 0;
};
#endif
