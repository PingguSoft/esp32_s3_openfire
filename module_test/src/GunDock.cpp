
#include "GunDock.h"


void GunDock::process() {
    int ch;

    if (_stream->available()) {
        ch = _stream->read();
        debug_printf("%2X [%c]\n", ch, ch);

        switch (_state) {
            case STATE_IDLE:
                _state = (ch == 'X') ? STATE_HEADER_START : STATE_IDLE;
                break;

            case STATE_HEADER_START:
                switch (ch) {
                    case 'B':  // Set IR Brightness
                        _state = STATE_BRIGHT;
                        break;

                    case 'T':  // Toggle Test/Processing Mode
                        break;

                    case 'P':  // Enter Docked Mode
                        if (_callback) {
                            _callback->onCallback(CMD_DOCK_MODE, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'E':  // Exit Docked Mode
                        break;

                    case 'C':  // Enter Calibration mode (optional: switch to cal profile if detected)
                        break;

                    case 'S':  // Save current profile
                        break;

                    case 'c':  // Clear EEPROM
                        break;

                    case 'm':  // Mapping new values to commit to EEPROM.
                        break;

                    case 'l':  // Print EEPROM values.
                        _state = STATE_EEPROM;
                        break;
                }
                break;

            case STATE_BRIGHT: {
                if (_callback) {
                    _callback->onCallback(CMD_IR_BRIGHTNESS, (uint8_t*)&ch, 1, _stream);
                }
                _state = STATE_IDLE;
            } break;

            case STATE_EEPROM: {
                switch (ch) {
                    case 'b':
                        if (_callback) {
                            _callback->onCallback(CMD_EEPROM_READ_TOGGLES, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'p':
                        if (_callback) {
                            _callback->onCallback(CMD_EEPROM_READ_PINS, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 's':
                        if (_callback) {
                            _callback->onCallback(CMD_EEPROM_READ_SETTINGS, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'P':
                        _state = STATE_EEPROM_PROFILE;
                        break;

                    case 'i':
                        if (_callback) {
                            _callback->onCallback(CMD_EEPROM_READ_USB, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;
                }
            } break;

            case STATE_EEPROM_PROFILE:
                if (_callback) {
                    _callback->onCallback(CMD_EEPROM_READ_PROFILE, (uint8_t*)&ch, 1, _stream);
                }
                _state = STATE_IDLE;
                break;
        }
    }
}