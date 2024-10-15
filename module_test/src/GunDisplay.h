#ifndef __GUN_DISPLAY_H__
#define __GUN_DISPLAY_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <Wire.h>
#include <arduino-timer.h>

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

class GunMenuCallback {
   public:
    typedef enum {
        ON_INIT = 0,
        ON_TOP_SEL, ON_TOP_DESEL, ON_TOP_VAL_CHANGE, ON_TOP_CLICK,
        ON_SUB_SEL, ON_SUB_DESEL, ON_SUB_VAL_CHANGE, ON_SUB_CLICK } op_t;
    virtual char *onMenuCallback(op_t op, int8_t top, int8_t sub, void *data) = 0;
};

class GunDisplay {
   public:
    typedef enum { TYPE_NONE = 0, TYPE_LIST, TYPE_BOOL, TYPE_DIGIT_8, TYPE_DIGIT_16 } type_t;
    // typedef enum { FLAG_NONE = 0, FLAG_HALF_LEFT = _BV(0), FLAG_HALF_RIGHT = _BV(1) } flag_t;

    typedef struct {
        char   *top;
        uint8_t type;
        uint8_t cnt;
        char   *subs[4];

        // internal
        struct {
            int8_t  sel;
            void   *data;
            uint16_t min;
            uint16_t max;
        } internal;
    } menu_t;

    typedef struct {
        char   *title;
        int8_t  top_idx;
        int8_t  sub_idx;
        uint8_t size;
        menu_t *menu;
        GunMenuCallback *callback;
    } menu_info_t;

    typedef enum { KEY_NONE = 0, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_BACK } key_t;

    GunDisplay();
    bool setup(TwoWire *wire);
    void loop();

    void TopPanelUpdate(char *textPrefix, char *textInput);
    void ScreenModeChange(int8_t screenMode, bool isAnalog = false, bool isBT = false);
    void IdleOps();
    void DrawVisibleIR(int pointX[4], int pointY[4]);
    void drawAmmo(uint8_t ammo);
    void drawLife(uint8_t life);


    menu_info_t *init_menu(char *title, menu_t *menu, uint8_t sz, GunMenuCallback *callback=NULL);
    void         set_menu_data(menu_info_t *mi, int8_t top, void *data) { mi->menu[top].internal.data = data; }
    void         set_menu_data_range(menu_info_t *mi, int8_t top, uint16_t min, uint16_t max) { mi->menu[top].internal.min = min; mi->menu[top].internal.max = max;}
    void         set_menu_sel(menu_info_t *mi, int8_t top, int8_t sel) { mi->menu[top].internal.sel = sel; }
    void         set_menu_subs(menu_info_t *mi, int8_t top, int8_t sub, char *text) { mi->menu[top].subs[sub] = text; }
    void         handle_menu(menu_info_t *mi, key_t key=KEY_NONE);

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
    GunMenuCallback                *_callback;

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
    void draw_menu(menu_info_t *mi);
    void draw_centered_text(char *text, uint8_t flag=0);
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], uint16_t color);
    void tokenize(char *line, char *token, std::vector<char *> &tokens);

    char *alloc_tmp(char *loc, int sz, char *text, uint8_t extra);
    void  free_tmp(char *loc, char *buf);

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
