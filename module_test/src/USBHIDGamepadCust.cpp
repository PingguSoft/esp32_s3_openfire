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
#include "USBHID.h"

#if CONFIG_TINYUSB_HID_ENABLED

#include "USBHIDGamepadCust.h"

// Gamepad Report Descriptor Template
// with 32 buttons, 2 joysticks and 1 hat/dpad with following layout
// | X | Y (1 byte each) | hat/DPAD (1 byte) | Button Map (1 bytes) |
#define TUD_HID_REPORT_DESC_GAMEPAD_CUST(...)                                                    \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),                \
        HID_COLLECTION(HID_COLLECTION_APPLICATION), /* Report ID if any */                       \
        __VA_ARGS__                                 /* 8 bit X, Y (min -127, max 127 ) */        \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                                  \
        HID_USAGE(HID_USAGE_DESKTOP_X), HID_USAGE(HID_USAGE_DESKTOP_Y), HID_LOGICAL_MIN(0x81),   \
        HID_LOGICAL_MAX(0x7f), HID_REPORT_COUNT(2), HID_REPORT_SIZE(8),                          \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), /* 8 bit DPad/Hat Button Map  */      \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),         \
        HID_LOGICAL_MIN(1), HID_LOGICAL_MAX(8), HID_PHYSICAL_MIN(0), HID_PHYSICAL_MAX_N(315, 2), \
        HID_REPORT_COUNT(1), HID_REPORT_SIZE(8),                                                 \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), /* 32 bit Button Map */               \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON), HID_USAGE_MIN(1), HID_USAGE_MAX(8),               \
        HID_LOGICAL_MIN(0), HID_LOGICAL_MAX(1), HID_REPORT_COUNT(8), HID_REPORT_SIZE(1),         \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), HID_COLLECTION_END

typedef struct TU_ATTR_PACKED {
    int8_t  x;        ///< Delta x  movement of left analog-stick
    int8_t  y;        ///< Delta y  movement of left analog-stick
    uint8_t hat;      ///< Buttons mask for currently pressed buttons in the DPad/hat
    uint8_t buttons;  ///< Buttons mask for currently pressed buttons
} hid_gamepad_report_cust_t;

static const uint8_t report_descriptor[] = {
    TUD_HID_REPORT_DESC_GAMEPAD_CUST(HID_REPORT_ID(HID_REPORT_ID_GAMEPAD))};

USBHIDGamepadCust::USBHIDGamepadCust()
    : hid(), _x(0), _y(0), _z(0), _rz(0), _rx(0), _ry(0), _hat(0), _buttons(0) {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        hid.addDevice(this, sizeof(report_descriptor));
    }
}

uint16_t USBHIDGamepadCust::_onGetDescriptor(uint8_t* buffer) {
    memcpy(buffer, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

void USBHIDGamepadCust::begin() {
    hid.begin();
}

void USBHIDGamepadCust::end() {
}

bool USBHIDGamepadCust::write() {
    hid_gamepad_report_cust_t report = {.x = _x, .y = _y, .hat = _hat, .buttons = _buttons};
    return hid.SendReport(HID_REPORT_ID_GAMEPAD, &report, sizeof(report));
}

bool USBHIDGamepadCust::leftStick(int8_t x, int8_t y) {
    _x = x;
    _y = y;
    return write();
}

bool USBHIDGamepadCust::rightStick(int8_t z, int8_t rz) {
    _z  = z;
    _rz = rz;
    return write();
}

bool USBHIDGamepadCust::leftTrigger(int8_t rx) {
    _rx = rx;
    return write();
}

bool USBHIDGamepadCust::rightTrigger(int8_t ry) {
    _ry = ry;
    return write();
}

bool USBHIDGamepadCust::hat(uint8_t hat) {
    if (hat > 9) {
        return false;
    }
    _hat = hat;
    return write();
}

bool USBHIDGamepadCust::pressButton(uint8_t button) {
    if (button > 31) {
        return false;
    }
    _buttons |= (1 << button);
    return write();
}

bool USBHIDGamepadCust::releaseButton(uint8_t button) {
    if (button > 31) {
        return false;
    }
    _buttons &= ~(1 << button);
    return write();
}

bool USBHIDGamepadCust::send(int8_t x, int8_t y, uint8_t hat, uint8_t buttons) {
    if (hat > 9) {
        return false;
    }
    _x       = x;
    _y       = y;
    _hat     = hat;
    _buttons = buttons;
    return write();
}

#endif /* CONFIG_TINYUSB_HID_ENABLED */
