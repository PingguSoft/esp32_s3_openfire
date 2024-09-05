#include "GunHID.h"
#include "USB.h"
#include "USBHIDGamepadCust.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include "debug.h"

static USBHIDKeyboard    *_keyboard;
static USBHIDMouse       *_mouse;
static USBHIDGamepadCust *_gamepad;
static USBCDC            *_usbserial;
static bool               _is_serial_connected = false;

static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id,
                             void *event_data) {
    if (event_base == ARDUINO_USB_EVENTS) {
        arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
        switch (event_id) {
            case ARDUINO_USB_STARTED_EVENT:
                break;
            case ARDUINO_USB_STOPPED_EVENT:
                break;
            case ARDUINO_USB_SUSPEND_EVENT:
                break;
            case ARDUINO_USB_RESUME_EVENT:
                break;

            default:
                break;
        }
    } else if (event_base == ARDUINO_USB_CDC_EVENTS) {
        arduino_usb_cdc_event_data_t *data = (arduino_usb_cdc_event_data_t *)event_data;
        switch (event_id) {
            case ARDUINO_USB_CDC_CONNECTED_EVENT:
                _is_serial_connected = true;
                LOGV("Connected !!!\n");
                break;
            case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
                _is_serial_connected = false;
                LOGV("Disconnected !!!\n");
                break;
            case ARDUINO_USB_CDC_LINE_STATE_EVENT:
                break;
            case ARDUINO_USB_CDC_LINE_CODING_EVENT:
                break;
            case ARDUINO_USB_CDC_RX_EVENT:
                break;
            case ARDUINO_USB_CDC_RX_OVERFLOW_EVENT:
                break;

            default:
                break;
        }
    } else if (event_base == ARDUINO_USB_HID_EVENTS) {
        arduino_usb_hid_event_data_t *data = (arduino_usb_hid_event_data_t *)event_data;
        switch (event_id) {
            case ARDUINO_USB_HID_SET_PROTOCOL_EVENT:
                break;
            case ARDUINO_USB_HID_SET_IDLE_EVENT:
                break;

            default:
                break;
        }
    }
}

void GunHIDUSB::setup() {
    _keyboard = new USBHIDKeyboard();
    _mouse     = new USBHIDMouse();
    _gamepad   = new USBHIDGamepadCust();
    _usbserial = new USBCDC();

    _usbserial->onEvent(usbEventCallback);
    _usbserial->begin(115200);
    _usbserial->enableReboot(true);

    _mouse->begin();
    _keyboard->begin();
    _gamepad->begin();

    USB.PID(_pid);
    USB.VID(_vid);
    USB.productName(_devName.c_str());
    USB.manufacturerName(_devMfr.c_str());
    USB.begin();
    _timer = new Timer<2>();
    _is_serial_connected = false;

    LOGV("setup !!!\n");
}

void GunHIDUSB::report_gamepad(int x, int y, uint8_t hat, uint8_t buttons) {
    _gamepad->send(x, y, hat, buttons);
}

bool GunHIDUSB::test(void *param) {
    static int state = 1;

    if (state) {
        _gamepad->leftStick(100, 100);
        _gamepad->pressButton(0);
        // _mouse->move(50, 50);
        // _keyboard->pressRaw(HID_KEY_CAPS_LOCK);
        // _keyboard->releaseRaw(HID_KEY_CAPS_LOCK);
    } else {
        _gamepad->leftStick(0, 0);
        _gamepad->releaseButton(0);
        // _mouse->move(-50, -50);
    }
    state = !state;

    return true;
}

void GunHIDUSB::loop() {
    // int ch;

    // if (_usbserial->available()) {
    //     ch = _usbserial->read();

    //     switch (ch) {
    //         case '1':
    //             _usbserial->printf("1\n");
    //             _tmr_handle = _timer->every(200, GunHIDUSB::test, NULL);
    //             break;

    //         case '2':
    //             _usbserial->printf("2\n");
    //             _timer->cancel(_tmr_handle);
    //             break;
    //     }
    // }
    _timer->tick();
}

Stream *GunHIDUSB::get_serial() {
    return _is_serial_connected ? (Stream*)_usbserial : &Serial0;
    // return (Stream*)&Serial0;
}
