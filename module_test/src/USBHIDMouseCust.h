/*
  Mouse.h

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

#pragma once
#include "USBHID.h"
#if CONFIG_TINYUSB_HID_ENABLED

class USBHIDMouseCust : public USBHIDDevice {
   private:
    typedef struct TU_ATTR_PACKED {
        uint8_t buttons; /**< buttons mask for currently pressed buttons in the mouse. */
        uint16_t x;       /**< Current delta x movement of the mouse. */
        uint16_t y;       /**< Current delta y movement on the mouse. */
    } mouse_report_cust_t;

    USBHID             hid;
    mouse_report_cust_t _report;

   public:
    USBHIDMouseCust(void);
    void begin(void);
    void end(void);
    void report(uint16_t x, uint16_t y, uint8_t buttons);
    bool isPressed(uint8_t b);

    // internal use
    uint16_t _onGetDescriptor(uint8_t* buffer);
};

#endif
