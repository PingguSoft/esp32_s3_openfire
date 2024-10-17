#ifndef __GUN_DISPLAY_H__
#define __GUN_DISPLAY_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <vector>

#include <Wire.h>
#include <arduino-timer.h>

#include "GunMenu.h"
#include "config.h"
#include "debug.h"


/*
*****************************************************************************************
* GunDisplay
*****************************************************************************************
*/
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

class GunDisplay {
   public:
    GunDisplay() {}
    bool     setup(TwoWire *wire);
    void     loop();

    void TopPanelUpdate(char *textPrefix, char *textInput);
    void ScreenModeChange(int8_t screenMode, bool isAnalog = false, bool isBT = false);
    void IdleOps();
    void DrawVisibleIR(int pointX[4], int pointY[4]);
    void drawAmmo(uint8_t ammo);
    void drawLife(uint8_t life);
    void draw_menu(GunMenu *menu);

    enum ScreenMode_e {
        Screen_None = -1,
        Screen_Init = 0,
        Screen_Normal,
        Screen_Pause,
        Screen_Profile,
        Screen_Saving,
        Screen_SaveSuccess,
        Screen_SaveError,
        Screen_Calibrating,
        Screen_IRTest,
        Screen_Docked,
        Screen_Mamehook_Single,
        Screen_Mamehook_Dual
    };

    enum ScreenPauseList_e {
        ScreenPause_Calibrate = 0,
        ScreenPause_ProfileSelect,
        ScreenPause_Save,
        ScreenPause_Rumble,
        ScreenPause_Solenoid,
        ScreenPause_EscapeKey
    };

    enum ScreenSerialInit_e { ScreenSerial_None = 0, ScreenSerial_Life, ScreenSerial_Ammo, ScreenSerial_Both };

    /// @brief Whether life updates are in lifebar or life glyphs form
    bool lifeBar = false;

    /// @brief Which layout we use for serial mode
    uint8_t serialDisplayType = 0;

   private:
    Timer<1, millis, GunDisplay *> *_timer;

    int8_t screenState = Screen_None;

    bool ammoEmpty = false;
    bool lifeEmpty = false;

    uint8_t currentAmmo;
    uint8_t currentLife;

    // timestamps, in case we need them for periodic tasks in IdleOps()
    unsigned long ammoTimestamp = 0;
    unsigned long lifeTimestamp = 0;
    unsigned long idleTimeStamp = 0;

    void draw_centered_text(char *text, uint8_t flag = 0);
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], uint16_t color);
    void tokenize(char *line, char *token, std::vector<char *> &tokens);
};

#endif
