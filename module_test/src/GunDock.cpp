
#include "GunDock.h"

void GunDock::process() {
    int ch;
    int  len = _stream->available();

    for (int i = 0; i < len; i++) {
        ch = _stream->read();
        // debug_printf("%2X [%c]\n", ch, ch);

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
                        if (_callback) {
                            _callback->onDockCallback(CMD_TOGGLE_TEST_MODE, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'P':  // Enter Docked Mode
                        if (_callback) {
                            ch = 1;
                            _callback->onDockCallback(CMD_DOCK_MODE, (uint8_t*)&ch, 1, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'E':  // Exit Docked Mode
                        if (_callback) {
                            ch = 0;
                            _callback->onDockCallback(CMD_DOCK_MODE, (uint8_t*)&ch, 1, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'C':  // Enter Calibration mode (optional: switch to cal profile if detected)
                        _state = STATE_CALIBRATION_IDX;
                        break;

                    case 'S':  // Save current profile
                        if (_callback) {
                            _callback->onDockCallback(CMD_SAVE_PREFERENCE, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'c':  // Clear EEPROM
                        if (_callback) {
                            _callback->onDockCallback(CMD_CLEAR_EEPROM, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'm':  // Mapping new values to commit to EEPROM.
                        _state = STATE_MAPPING;
                        break;

                    case 'l':  // Print EEPROM values.
                        _state = STATE_EEPROM;
                        break;

                    case 't':  // force feedback test
                        _state = STATE_TEST_FFB;
                        break;

                    case 'x':  // reboot
                        _state = STATE_REBOOT;
                        break;
                }
                break;

            case STATE_REBOOT:
                if (ch == 'x') {
                    if (_callback) {
                        _callback->onDockCallback(CMD_REBOOT, NULL, 0, _stream);
                    }
                }
                _state = STATE_IDLE;
                break;

            case STATE_TEST_FFB:
                if (_callback) {
                    _callback->onDockCallback(CMD_TEST_FFB, (uint8_t*)&ch, 1, _stream);
                }
                _state = STATE_IDLE;
                break;

            case STATE_BRIGHT:
                if (_callback) {
                    _callback->onDockCallback(CMD_IR_BRIGHTNESS, (uint8_t*)&ch, 1, _stream);
                }
                _state = STATE_IDLE;
                break;

            case STATE_CALIBRATION_IDX:
                _buf[0] = ch;
                _state  = STATE_CALIBRATION;
                break;

            case STATE_CALIBRATION:
                if (ch == 'C') {
                    if (_callback) {
                        _callback->onDockCallback(CMD_CALIBRATION_MODE, _buf, 1, _stream);
                    }
                }
                _state = STATE_IDLE;
                break;

            case STATE_MAPPING:
                if (ch == '.') {
                    _pos   = 0;
                    _state = STATE_MAPPING_DATA;
                } else if (ch == 'X') {
                    _state = STATE_HEADER_START;
                } else {
                    _state = STATE_IDLE;
                }
                break;

            case STATE_MAPPING_DATA:
                if (ch == 'X' || ch == '.') {
                    if (_callback) {
                        _buf[_pos++] = 0;
                        _callback->onDockCallback(CMD_EEPROM_UPDATE, (uint8_t*)_buf, _pos, _stream);
                    }
                    _state = (ch == 'X') ? STATE_HEADER_START : STATE_IDLE;
                } else {
                    _buf[_pos++] = ch;
                }
                break;

            case STATE_EEPROM:
                switch (ch) {
                    case 'b':
                        if (_callback) {
                            _callback->onDockCallback(CMD_EEPROM_READ_TOGGLES, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'p':
                        if (_callback) {
                            _callback->onDockCallback(CMD_EEPROM_READ_PINS, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 's':
                        if (_callback) {
                            _callback->onDockCallback(CMD_EEPROM_READ_SETTINGS, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'P':
                        _state = STATE_EEPROM_PROFILE;
                        break;

                    case 'i':
                        if (_callback) {
                            _callback->onDockCallback(CMD_EEPROM_READ_USB, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;
                }
                break;

            case STATE_EEPROM_PROFILE:
                if (_callback) {
                    _callback->onDockCallback(CMD_EEPROM_READ_PROFILE, (uint8_t*)&ch, 1, _stream);
                }
                _state = STATE_IDLE;
                break;
        }
    }
}