#ifndef __GUN_PREFERENCES_H__
#define __GUN_PREFERENCES_H__

#include <Arduino.h>

class GunSettings {
   public:
/*
*****************************************************************************************
* CONST
*****************************************************************************************
*/
#define MAX_PROFILE_CNT 4

    enum ButtonIndex_e {
        BtnIdx_Trigger = 0,
        BtnIdx_A,
        BtnIdx_B,
        BtnIdx_Start,
        BtnIdx_Select,
        BtnIdx_Up,
        BtnIdx_Down,
        BtnIdx_Left,
        BtnIdx_Right,
        BtnIdx_Reload,
        BtnIdx_Pedal,
        BtnIdx_Pedal2,
        BtnIdx_Pump,
        BtnIdx_Home
    };

    // bit mask for each button, must match ButtonDesc[] order to match the proper button events
    enum ButtonMask_e {
        BtnMask_Trigger = 1 << BtnIdx_Trigger,
        BtnMask_A       = 1 << BtnIdx_A,
        BtnMask_B       = 1 << BtnIdx_B,
        BtnMask_Start   = 1 << BtnIdx_Start,
        BtnMask_Select  = 1 << BtnIdx_Select,
        BtnMask_Up      = 1 << BtnIdx_Up,
        BtnMask_Down    = 1 << BtnIdx_Down,
        BtnMask_Left    = 1 << BtnIdx_Left,
        BtnMask_Right   = 1 << BtnIdx_Right,
        BtnMask_Reload  = 1 << BtnIdx_Reload,
        BtnMask_Pedal   = 1 << BtnIdx_Pedal,
        BtnMask_Pedal2  = 1 << BtnIdx_Pedal2,
        BtnMask_Pump    = 1 << BtnIdx_Pump,
        BtnMask_Home    = 1 << BtnIdx_Home
    };

    enum RunMode_e {
        RunMode_Normal     = 0,  ///< Normal gun mode, no averaging
        RunMode_Average    = 1,  ///< 2 frame moving average
        RunMode_Average2   = 2,  ///< weighted average with 3 frames
        RunMode_ProfileMax = 2,  ///< maximum mode allowed for profiles
        RunMode_Processing = 3,  ///< Processing test mode
        RunMode_Count
    };

    enum GunMode_e {
        GunMode_Init = -1,
        GunMode_Run  = 0,
        GunMode_Calibration,
        GunMode_Verification,
        GunMode_Pause,
        GunMode_Docked
    };

    /*
    *****************************************************************************************
    * DATA TYPES
    *****************************************************************************************
    */
    /// @brief Header ID
    typedef union HeaderId_u {
        uint8_t  bytes[4];
        uint32_t u32;
    } __attribute__((packed)) HeaderId_t;

    /// @brief Profile data
    typedef struct ProfileData_s {
        int      topOffset;  // Perspective: Offsets
        int      bottomOffset;
        int      leftOffset;
        int      rightOffset;
        float    TLled;  // Perspective: LED relative anchors
        float    TRled;
        float    adjX;  // Perspective: adjusted axis
        float    adjY;
        uint32_t irSensitivity : 3;  // IR Sensitivity from 0-2
        uint32_t runMode : 5;        // Averaging mode
        uint32_t buttonMask : 16;    // Button mask assigned to this profile
        bool     irLayout;           // square or diamond IR for this display?
        uint32_t color : 24;         // packed color blob per profile
        char     name[16];           // Profile display name
    } __attribute__((packed)) ProfileData_t;

    /// @brief Preferences that can be stored in flash
    typedef struct Preferences_s {
        uint8_t       profileCount;
        uint8_t       selectedProfile;
        ProfileData_t profileData[MAX_PROFILE_CNT];
    } __attribute__((packed)) Preferences_t;

    typedef struct TogglesMap_s {
        bool customPinsInUse;  // Are we using custom pins mapping?
        bool rumbleActive;    // Are we allowed to do rumble?
        bool solenoidActive;  // Are we allowed to use a solenoid?
        bool autofireActive;  // Is autofire enabled?
        bool simpleMenu;      // Is simple pause menu active?
        bool holdToPause;     // Is holding A/B buttons to enter pause mode allowed?
        bool commonAnode;  // If LED is Common Anode (+, connects to 5V) rather than Common Cathode (-, connects to GND)
        bool lowButtonMode;   // Is low buttons mode active?
        bool rumbleFF;        // Rumble force-feedback, instead of Solenoid
    } __attribute__((packed)) TogglesMap_t;

    typedef struct PinsMap_s {
        int8_t bTrigger;     // Trigger
        int8_t bGunA;        // Button A (GunCon 1/Stunner/Justifier)
        int8_t bGunB;        // Button B (GunCon 1)
        int8_t bStart;       // Start Button (GCon-2)
        int8_t bSelect;      // Select Button (GCon-2)
        int8_t bGunUp;       // D-Pad Up (GCon-2)
        int8_t bGunDown;     // D-Pad Down (GCon-2)
        int8_t bGunLeft;     // D-Pad Left (GCon-2)
        int8_t bGunRight;    // D-Pad Right (GCon-2)
        int8_t bGunC;        // Button C (GCon-2)
        int8_t bPedal;       // External Pedal (DIY)
        int8_t bPedal2;      // External Pedal 2 (DIY)
        int8_t bHome;        // Home Button (Top Shot Elite)
        int8_t bPump;        // Pump Action Reload Button (Top Shot Elite)
        int8_t oRumble;      // Rumble Signal Pin
        int8_t oSolenoid;    // Solenoid Signal Pin
        int8_t sRumble;      // Rumble Switch
        int8_t sSolenoid;    // Solenoid Switch
        int8_t sAutofire;    // Autofire Switch
        int8_t oPixel;       // Custom NeoPixel Pin
        int8_t oLedR;        // 4-Pin RGB Red Pin
        int8_t oLedB;        // 4-Pin RGB Blue Pin
        int8_t oLedG;        // 4-Pin RGB Green Pin
        int8_t pCamSDA;      // Camera I2C Data Pin
        int8_t pCamSCL;      // Camera I2C Clock Pin
        int8_t pPeriphSDA;   // Other I2C Peripherals Data Pin
        int8_t pPeriphSCL;   // Other I2C Peripherals Clock Pin
        int8_t aBattRead;    // Battery voltage circuit thingy?
        int8_t aStickX;      // Analog Stick X-axis
        int8_t aStickY;      // Analog Stick Y-axis
        int8_t aTMP36;       // Analog TMP36 Temperature Sensor Pin
    } PinsMap_t;

    typedef struct SettingsMap_s {
        uint8_t  rumbleIntensity;
        uint16_t rumbleInterval;
        uint16_t solenoidNormalInterval;
        uint16_t solenoidFastInterval;
        uint16_t solenoidLongInterval;
        uint8_t  autofireWaitFactor;
        uint16_t pauseHoldLength;
        uint8_t  customLEDcount;
        uint8_t  customLEDstatic;
        uint32_t customLEDcolor1;
        uint32_t customLEDcolor2;
        uint32_t customLEDcolor3;
    } SettingsMap_t;

    typedef struct USBMap_s {
        char     deviceName[16];
        uint16_t devicePID;
    } USBMap_t;

    typedef struct {
        uint16_t auto_trg_delay;
        uint16_t auto_trg_rpt_delay;
        uint8_t  recoil_type;
        uint8_t  recoil_pwr;
    } __attribute__((packed)) pref_device_t;

    static TogglesMap_t     _toggles;
    static PinsMap_t        _pins;
    static SettingsMap_t    _settings;
    static USBMap_t         _usb;

    /*
    *****************************************************************************************
    * FUNCTIONS
    *****************************************************************************************
    */
    GunSettings() { _gunmode = GunMode_Init; }

    void           set_gun_mode(GunMode_e mode) { _gunmode = mode; }
    GunMode_e      get_gun_mode() { return _gunmode; }
    Preferences_t* get_preference() { return &_preferences; }
    ProfileData_t* get_profile_data(uint8_t idx) { return &_preferences.profileData[idx]; }
    ProfileData_t* get_profile_data() { return &_preferences.profileData[_preferences.selectedProfile]; }
    void           set_cur_profile(uint8_t sel) { _preferences.selectedProfile = sel; }
    uint8_t        get_cur_profile() { return _preferences.selectedProfile; }

    void setup();
    void save();
    bool load();

    /// @brief Required size for the preferences
    static unsigned int Size() { return sizeof(HeaderId_u) + sizeof(_preferences); }

   private:
    void dump();

    GunMode_e _gunmode;

    // header ID to ensure junk isn't loaded if preferences aren't saved
    static const HeaderId_t _HeaderId;
    static Preferences_t    _preferences;  // single instance of the preference data
};

#endif
