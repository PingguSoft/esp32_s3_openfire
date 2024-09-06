/*
  Mouse.cpp

  Copyright (c) 2015, Arduino LLC
  Original code (pre-library): Copyright (c) 2011, Peter Barrett

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "USBHID.h"

#if CONFIG_TINYUSB_HID_ENABLED

#include "USBHIDMouseCust.h"

// Mouse Report Descriptor Template
#define TUD_HID_REPORT_DESC_MOUSE_CUST(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP      )                   ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_MOUSE     )                   ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION  )                   ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    HID_USAGE      ( HID_USAGE_DESKTOP_POINTER )                   ,\
    HID_COLLECTION ( HID_COLLECTION_PHYSICAL   )                   ,\
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_BUTTON  )                   ,\
        HID_USAGE_MIN   ( 1                                      ) ,\
        HID_USAGE_MAX   ( 5                                      ) ,\
        HID_LOGICAL_MIN ( 0                                      ) ,\
        HID_LOGICAL_MAX ( 1                                      ) ,\
        /* Left, Right, Middle, Backward, Forward buttons */ \
        HID_REPORT_COUNT( 5                                      ) ,\
        HID_REPORT_SIZE ( 1                                      ) ,\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
        /* 3 bit padding */ \
        HID_REPORT_COUNT( 1                                      ) ,\
        HID_REPORT_SIZE ( 3                                      ) ,\
        HID_INPUT       ( HID_CONSTANT                           ) ,\
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP )                   ,\
        /* X, Y position [0, 32767] */ \
        HID_USAGE       ( HID_USAGE_DESKTOP_X                    ) ,\
        HID_USAGE       ( HID_USAGE_DESKTOP_Y                    ) ,\
        HID_LOGICAL_MIN_N ( 0x0, 2                               ) ,\
        HID_LOGICAL_MAX_N ( 0x7fff, 2                            ) ,\
        HID_REPORT_COUNT( 2                                      ) ,\
        HID_REPORT_SIZE ( 16                                     ) ,\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    HID_COLLECTION_END                                            , \
  HID_COLLECTION_END \

static const uint8_t report_descriptor[] = {
    TUD_HID_REPORT_DESC_MOUSE_CUST(HID_REPORT_ID(HID_REPORT_ID_MOUSE))
};

USBHIDMouseCust::USBHIDMouseCust() : hid() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        hid.addDevice(this, sizeof(report_descriptor));
    }
}

uint16_t USBHIDMouseCust::_onGetDescriptor(uint8_t* dst) {
    memcpy(dst, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

void USBHIDMouseCust::begin() {
    hid.begin();
}

void USBHIDMouseCust::end() {
}

void USBHIDMouseCust::report(uint16_t x, uint16_t y, uint8_t buttons) {
    _report.x       = x;
    _report.y       = y;
    _report.buttons = buttons;
    if (hid.ready()) {
        hid.SendReport(HID_REPORT_ID_MOUSE, &_report, sizeof(_report));
    }
}

bool USBHIDMouseCust::isPressed(uint8_t b) {
    if ((b & _report.buttons) > 0) {
        return true;
    }
    return false;
}

#endif /* CONFIG_TINYUSB_HID_ENABLED */
