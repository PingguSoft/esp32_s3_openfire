#ifndef __GUN_DISPLAY_H__
#define __GUN_DISPLAY_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <arduino-timer.h>
#include <Wire.h>
#include "config.h"
#include "debug.h"

/*
*****************************************************************************************
* GunDisplay
*****************************************************************************************
*/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class GunDisplay {
   public:
    typedef struct {
        char    *top;
        uint8_t cnt;
        char    *subs[];
    } menu_t;

    typedef struct {
        char    *title;
        uint8_t top_idx;
        uint8_t sub_idx;
        uint8_t size;
        menu_t  *menu;
    } menu_info_t;

    GunDisplay();
    bool setup(TwoWire *wire);
    void loop();

    void TopPanelUpdate(char *textPrefix, char *textInput);
    void ScreenModeChange(int8_t screenMode, bool isAnalog = false, bool isBT = false);
    void IdleOps();
    void DrawVisibleIR(int pointX[4], int pointY[4]);
    void drawAmmo(uint8_t ammo);
    void drawLife(uint8_t life);

    void draw_menu(menu_info_t *mi);
    menu_info_t *init_menu(char *title, menu_t  *menu, uint8_t sz);
    void handle_menu(menu_info_t *mi, uint8_t key);

    enum menu_key_e {
        KEY_UP = 1,
        KEY_DOWN,
        KEY_SELECT,
        KEY_BACK
    };

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

    enum ScreenSerialInit_e {
        ScreenSerial_None = 0,
        ScreenSerial_Life,
        ScreenSerial_Ammo,
        ScreenSerial_Both
    };

    /// @brief Whether life updates are in lifebar or life glyphs form
    bool lifeBar = false;

    /// @brief Which layout we use for serial mode
    uint8_t serialDisplayType = 0;

   private:
    Timer<1, millis, GunDisplay *>     *_timer;

    int8_t screenState = Screen_None;

    bool ammoEmpty = false;
    bool lifeEmpty = false;

    uint8_t currentAmmo;
    uint8_t currentLife;

    // timestamps, in case we need them for periodic tasks in IdleOps()
    unsigned long ammoTimestamp = 0;
    unsigned long lifeTimestamp = 0;
    unsigned long idleTimeStamp = 0;

    void draw_top_menu(menu_info_t *mi);
    void draw_sub_menu(menu_info_t *mi);
    void draw_centered_text(char *text);
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], uint16_t color);

#if 0
    typedef struct {
        bool        inverse;
        uint8_t     text_size;
        uint16_t    x;
        uint16_t    y;
        char        *text;
    } text_t;
    void draw_menu(text_t *menu);
#endif

};

#endif
