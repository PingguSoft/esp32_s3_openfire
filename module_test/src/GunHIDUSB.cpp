#include "GunHID.h"
#include "USB.h"
#include "USBHIDGamepadCust.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouseCust.h"
#include "debug.h"

static USBHIDKeyboard    *_keyboard;
static USBHIDMouseCust   *_mouse;
static USBHIDGamepadCust *_gamepad;
static USBCDC            *_usbserial;

static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == ARDUINO_USB_CDC_EVENTS) {
        arduino_usb_cdc_event_data_t *data = (arduino_usb_cdc_event_data_t *)event_data;

        switch (event_id) {
            case ARDUINO_USB_CDC_CONNECTED_EVENT:
                LOGV("CDC CONNECTED\n");
                break;

            case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
                LOGV("CDC DISCONNECTED\n");
                break;

            case ARDUINO_USB_CDC_LINE_STATE_EVENT:
                LOGV("CDC LINE STATE: dtr: %u, rts: %u\n", data->line_state.dtr, data->line_state.rts);
                // if (data->line_state.dtr)
                //     usb_persist_restart(RESTART_BOOTLOADER);
                //     esp_restart();
                break;

            case ARDUINO_USB_CDC_LINE_CODING_EVENT:
                LOGV("CDC LINE CODING: bit_rate: %lu, data_bits: %u, stop_bits: %u, parity: %u\n",
                     data->line_coding.bit_rate, data->line_coding.data_bits, data->line_coding.stop_bits,
                     data->line_coding.parity);
                break;

            case ARDUINO_USB_CDC_RX_EVENT:
                // LOGV("CDC RX [%u]\n", data->rx.len);
                break;

            case ARDUINO_USB_CDC_RX_OVERFLOW_EVENT:
                LOGV("CDC RX Overflow of %d bytes\n", data->rx_overflow.dropped_bytes);
                break;

            default:
                break;
        }
    }
}

void GunHIDUSB::setup() {
    _keyboard  = new USBHIDKeyboard();
    _mouse     = new USBHIDMouseCust();
    _gamepad   = new USBHIDGamepadCust();
    _usbserial = new USBCDC();

    _usbserial->onEvent(usbEventCallback);
    _usbserial->begin(9600);
    // _usbserial->enableReboot(true);

    _mouse->begin();
    _keyboard->begin();
    _gamepad->begin();

    USB.PID(_pid);
    USB.VID(_vid);
    USB.productName(_devName.c_str());
    USB.manufacturerName(_devMfr.c_str());
    USB.begin();
    LOGV("setup !!!\n");
}

void GunHIDUSB::report_gamepad(int x, int y, uint8_t hat, uint8_t buttons) {
    _gamepad->send(x, y, hat, buttons);
}

void GunHIDUSB::report_mouse(int x, int y, uint8_t buttons) {
    _mouse_x       = x;
    _mouse_y       = y;
    _mouse_buttons = buttons;
    _mouse->report(x, y, buttons);
}

Stream *GunHIDUSB::get_serial() {
    return (Stream *)_usbserial;
}
