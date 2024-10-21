#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
*****************************************************************************************
* FEATURES
*****************************************************************************************
*/

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/
#define JOY_ADC_MIN 500UL
#define JOY_ADC_MAX 2800UL

/*
*****************************************************************************************
* H/W CONSTANTS (PINS)
*****************************************************************************************
*/
#define PIN_IR_CLK    0
#define PIN_IR_RESET  45
#define PIN_IR_SDA    39
#define PIN_IR_SCL    38
#define PIN_LED_STRIP 48
#define PIN_RUMBLE    11
#define PIN_SOLENOID  12
#define PIN_PERI_SDA  21
#define PIN_PERI_SCL  47

// buttons
#define PIN_JOY_ADC_X     4
#define PIN_JOY_ADC_Y     5
#define PIN_BUTTON_SELECT 9
#define PIN_BUTTON_START  10
#define PIN_BUTTON_PEDAL  13

#define PIN_BUTTON_A      6
#define PIN_BUTTON_B      7
#define PIN_BUTTON_C      15
#define PIN_TRIGGER       14


// PAUSE mode : PIN_BUTTON_C + PIN_BUTTON_SELECT
// A, B, Start, Select: select a profile
// Start + Down: Normal gun mode (averaging disabled)
// Start + Up: Normal gun with averaging, switch between the 2 averaging modes (use serial monitor to see the setting)
// B + Down: Decrease IR camera sensitivity (use a serial monitor to see the setting)
// B + Up: Increase IR camera sensitivity (use a serial monitor to see the setting)
// C/Reload: Exit pause mode
// C/Reload + A: Toggle Offscreen Button Mode
// Left: Toggle Rumble (when no rumble switch is detected)
// Right: Toggle Solenoid (when no solenoid switch is detected)
// Trigger: Begin calibration
// Start + Select: save settings to non-volatile memory (EEPROM or Flash depending on the board configuration)




/*
*****************************************************************************************
* MACROS & STRUCTURES
*****************************************************************************************
*/

#endif