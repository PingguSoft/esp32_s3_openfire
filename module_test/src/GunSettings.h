#ifndef __GUN_PREFERENCES_H__
#define __GUN_PREFERENCES_H__

#include <Arduino.h>

#include <vector>

#include "GunDock.h"

#define OPENFIRE_VERSION  5.2
#define OPENFIRE_CODENAME "Dawn"
#define OPENFIRE_BOARD    "rpipico"

class GunSettings : public GunSerialCallback {
   public:

// clang-format off
/*
*****************************************************************************************
* CONST
*****************************************************************************************
*/
#define MAX_PROFILE_CNT 4
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
    typedef union HeaderId_u {
        uint8_t  bytes[4];
        uint32_t u32;
    } __attribute__((packed)) header_id_t;

    /// @brief Profile data
    typedef struct ProfileData_s {
        int      topOffset;         // Perspective: Offsets
        int      bottomOffset;
        int      leftOffset;
        int      rightOffset;
        float    TLled;             // Perspective: LED relative anchors
        float    TRled;
        float    adjX;              // Perspective: adjusted axis
        float    adjY;
        uint8_t  irSensitivity;     // IR Sensitivity from 0-2
        uint8_t  runMode;           // Averaging mode
        uint16_t buttonMask;        // Button mask assigned to this profile
        bool     irLayout;          // square or diamond IR for this display?
        uint32_t color;             // packed color blob per profile
        char     name[16];          // Profile display name
    } __attribute__((packed)) profile_data_t;

    /// @brief Preferences that can be stored in flash
    typedef struct Preferences_s {
        uint8_t        profileCount;
        uint8_t        selectedProfile;
        profile_data_t profileData[MAX_PROFILE_CNT];
    } __attribute__((packed)) profiles_t;

    typedef struct TogglesMap_s {
        bool customPinsInUse;   // Are we using custom pins mapping?
        bool rumbleActive;      // Are we allowed to do rumble?
        bool solenoidActive;    // Are we allowed to use a solenoid?
        bool autofireActive;    // Is autofire enabled?
        bool simpleMenu;        // Is simple pause menu active?
        bool holdToPause;       // Is holding A/B buttons to enter pause mode allowed?
        bool commonAnode;       // If LED is Common Anode (+, connects to 5V) rather than Common Cathode (-, connects to GND)
        bool lowButtonMode;     // Is low buttons mode active?
        bool rumbleFF;          // Rumble force-feedback, instead of Solenoid
        bool serialHooker;      // mamehooker?
    } __attribute__((packed)) feature_map_t;

    typedef struct PinsMap_s {
        int8_t bTrigger;    // Trigger
        int8_t bGunA;       // Button A (GunCon 1/Stunner/Justifier)
        int8_t bGunB;       // Button B (GunCon 1)
        int8_t bStart;      // Start Button (GCon-2)
        int8_t bSelect;     // Select Button (GCon-2)
        int8_t bGunUp;      // D-Pad Up (GCon-2)
        int8_t bGunDown;    // D-Pad Down (GCon-2)
        int8_t bGunLeft;    // D-Pad Left (GCon-2)
        int8_t bGunRight;   // D-Pad Right (GCon-2)
        int8_t bGunC;       // Button C (GCon-2)
        int8_t bPedal;      // External Pedal (DIY)
        int8_t bPedal2;     // External Pedal 2 (DIY)
        int8_t bHome;       // Home Button (Top Shot Elite)
        int8_t bPump;       // Pump Action Reload Button (Top Shot Elite)
        int8_t oRumble;     // Rumble Signal Pin
        int8_t oSolenoid;   // Solenoid Signal Pin
        int8_t sRumble;     // Rumble Switch
        int8_t sSolenoid;   // Solenoid Switch
        int8_t sAutofire;   // Autofire Switch
        int8_t oPixel;      // Custom NeoPixel Pin
        int8_t oLedR;       // 4-Pin RGB Red Pin
        int8_t oLedB;       // 4-Pin RGB Blue Pin
        int8_t oLedG;       // 4-Pin RGB Green Pin
        int8_t pCamSDA;     // Camera I2C Data Pin
        int8_t pCamSCL;     // Camera I2C Clock Pin
        int8_t pPeriphSDA;  // Other I2C Peripherals Data Pin
        int8_t pPeriphSCL;  // Other I2C Peripherals Clock Pin
        int8_t aBattRead;   // Battery voltage circuit thingy?
        int8_t aStickX;     // Analog Stick X-axis
        int8_t aStickY;     // Analog Stick Y-axis
        int8_t aTMP36;      // Analog TMP36 Temperature Sensor Pin
    } __attribute__((packed)) pins_map_t;

    typedef struct SettingsMap_s {
        uint8_t  rumbleIntensity;
        uint16_t rumbleInterval;
        uint8_t  solenoidIntensity;         // added
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
    } __attribute__((packed)) params_map_t;

    typedef struct USBMap_s {
        char     deviceName[16];
        uint16_t devicePID;
    } __attribute__((packed)) usb_map_t;
// clang-format on

    /*
    *****************************************************************************************
    * FUNCTIONS
    *****************************************************************************************
    */
    GunSettings() { _gunmode = GunMode_Run; }

    GunMode_e set_gun_mode(GunMode_e mode) {
        GunMode_e prev = _gunmode;

        _gunmode = mode;
        return prev;
    }
    GunMode_e get_gun_mode() { return _gunmode; }

    profile_data_t* get_profile(uint8_t idx) { return &_profiles.profileData[idx]; }
    profile_data_t* get_profile() { return &_profiles.profileData[_profiles.selectedProfile]; }
    void            set_profile_idx(uint8_t sel) { _profiles.selectedProfile = sel; }
    uint8_t         get_profile_idx() { return _profiles.selectedProfile; }

    feature_map_t*   get_feature_config() { return &_features; }
    params_map_t*    get_param_config() { return &_params; }
    usb_map_t*      get_usb_config() { return &_usb; }
    pins_map_t*     get_pin_config() { return &_pins; }

    bool setup();
    void save();
    bool load();

    virtual void onSerialCallback(uint8_t cmd, uint8_t* pData, uint16_t size, Stream* stream);

    /// @brief Required size for the preferences
    static unsigned int Size() { return sizeof(HeaderId_u) + sizeof(_profiles); }

   private:
    void dump();
    void tokenize(char* line, char* token, std::vector<char*>& tokens);

    GunMode_e _gunmode;
    uint8_t   _runmode_saved;

    // header ID to ensure junk isn't loaded if preferences aren't saved
    static const header_id_t _header_id;
    static profiles_t        _profiles;  // single instance of the preference data
    static feature_map_t     _features;
    static pins_map_t        _pins;
    static params_map_t      _params;
    static usb_map_t         _usb;
};

#endif
