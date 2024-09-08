#include "GunSettings.h"
#include "debug.h"

#include <Arduino.h>
#include <EEPROM.h>

// 4 byte header ID
const GunSettings::HeaderId_t GunSettings::_HeaderId = {'O', 'F', '0', '1'};

GunSettings::Preferences_t GunSettings::_preferences = {
    MAX_PROFILE_CNT,
    0,
    {
        {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, GunSettings::BtnMask_A,      true, 0xFF0000, "Profile A"},
        {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, GunSettings::BtnMask_B,      true, 0x00FF00, "Profile B"},
        {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, GunSettings::BtnMask_Start,  true, 0x0000FF, "Profile Start"},
        {0, 0, 0, 0, 500 << 2, 1420 << 2, 512 << 2, 384 << 2, 0, GunSettings::RunMode_Average, GunSettings::BtnMask_Select, true, 0xFF00FF, "Profile Select"}
    },
};

GunSettings::TogglesMap_t GunSettings::_toggles = {false, true, true, false, false, false, true, false, false};

GunSettings::PinsMap_t GunSettings::_pins = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

GunSettings::SettingsMap_t GunSettings::_settings = {255,  150, 45, 30,       500,      3,
                                                     2500, 1,   0,  0xFF0000, 0x00FF00, 0x0000FF};

GunSettings::USBMap_t GunSettings::_usb = {
    "SERIALREADERR01",
    0,
};

void GunSettings::setup() {
    EEPROM.begin(2048);
}

void GunSettings::save() {
    EEPROM.put(0, _HeaderId.u32);
    EEPROM.put(4, _preferences);
    EEPROM.commit();
 }

int hex2string(uint8_t *in, int inlen, char *out) {
    int i = 0;
    char *pos = out;

    for (i = 0; i < inlen; i++)
        pos += sprintf(pos, "%02X ", in[i]);

    return pos - out + 1;
}

void GunSettings::dump() {
    char *buf = (char *)malloc(4096);

    hex2string((uint8_t *)&_preferences, sizeof(_preferences), buf);
    LOGV("%s\n", buf);

    free(buf);
}

bool GunSettings::load() {
    uint32_t u32;

    EEPROM.get(0, u32);
    if(u32 == _HeaderId.u32) {
        EEPROM.get(4, _preferences);
        LOGV("loaded\n");
        return true;
    }
    LOGV("load fail\n");
    return false;
}