#include <BleCompositeHID.h>
#include <BleConnectionStatus.h>
#include <GamepadDevice.h>
#include <MouseDevice.h>

#include "GunHID.h"
#include "debug.h"

static GamepadDevice   *_gamepad;
static MouseDevice     *_mouse;
static BleCompositeHID *_bleCompositeHID;

void GunHIDBLE::setup() {
    // Set up _gamepad
    GamepadConfiguration gamepadConfig;
    gamepadConfig.setButtonCount(16);
    gamepadConfig.setHatSwitchCount(1);
    gamepadConfig.setWhichAxes(true, true, false, false, false, false, false, false);
    gamepadConfig.setAxesMin(-127);
    gamepadConfig.setAxesMax(127);
    gamepadConfig.setAutoReport(false);
    _gamepad = new GamepadDevice(gamepadConfig);

    // Set up _mouse
    _mouse = new MouseDevice();

    // Add both devices to the composite HID device to manage them
    _bleCompositeHID = new BleCompositeHID(_devName, _devMfr, 100);
    _bleCompositeHID->addDevice(_gamepad);
    _bleCompositeHID->addDevice(_mouse);

    // Start the composite HID device to broadcast HID reports
    _bleCompositeHID->begin();
    LOGV("setup !!!\n");
}

// bool GunHIDBLE::test(void *param) {
//     static int state = 1;

//     if (_bleCompositeHID->isConnected()) {
//         if (state) {
//             _gamepad->press(BUTTON_1);
//             _gamepad->press(BUTTON_3);
//             _gamepad->setAxes(127, 127, 0, 0, 0, 0, 0, 0);
//             _gamepad->setHat1(HAT_DOWN_RIGHT);
//             _gamepad->sendGamepadReport();
//         } else {
//             _gamepad->release(BUTTON_1);
//             _gamepad->setAxes(0, 0, 0, 0, 0, 0, 0, 0);
//             _gamepad->setHat1(HAT_CENTERED);
//             _gamepad->sendGamepadReport();
//         }
//         state = !state;
//     }

//     return true;
// }

