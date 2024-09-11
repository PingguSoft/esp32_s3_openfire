// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include "USBHID.h"
#if CONFIG_TINYUSB_HID_ENABLED

/// Standard Gamepad Buttons Naming from Linux input event codes
/// https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h


class USBHIDGamepadCust : public USBHIDDevice {
   private:
    USBHID  hid;
    int8_t  _x;        ///< Delta x  movement of left analog-stick
    int8_t  _y;        ///< Delta y  movement of left analog-stick
    int8_t  _z;        ///< Delta z  movement of right analog-joystick
    int8_t  _rz;       ///< Delta Rz movement of right analog-joystick
    int8_t  _rx;       ///< Delta Rx movement of analog left trigger
    int8_t  _ry;       ///< Delta Ry movement of analog right trigger
    uint8_t _hat;      ///< Buttons mask for currently pressed buttons in the DPad/hat
    uint16_t _buttons;  ///< Buttons mask for currently pressed buttons
    bool    write();

   public:
    USBHIDGamepadCust(void);
    void begin(void);
    void end(void);

    bool leftStick(int8_t x, int8_t y);
    bool rightStick(int8_t z, int8_t rz);

    bool leftTrigger(int8_t rx);
    bool rightTrigger(int8_t ry);

    bool hat(uint8_t hat);

    bool pressButton(uint16_t button);
    bool releaseButton(uint16_t button);

    bool send(int8_t x, int8_t y, uint8_t hat, uint16_t buttons);

    // internal use
    uint16_t _onGetDescriptor(uint8_t* buffer);
};

#endif
