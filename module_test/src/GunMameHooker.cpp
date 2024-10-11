
#include "GunMameHooker.h"

void GunMameHooker::process() {
    int ch;

    if (_stream->available()) {
        ch = _stream->read();
        // debug_printf("%2X [%c]\n", ch, ch);

        switch (_state) {
            case STATE_IDLE:
                switch (ch) {
                    case 'S':
                        _state = STATE_CMD_START;
                        break;

                    case 'E':
                        _state = STATE_CMD_END;
                        break;

                    case 'M':
                        _state = STATE_CMD_MODE;
                        break;

                    case 'F':
                        _state = STATE_CMD_FFB;
                        break;

                    default:
                        _state = STATE_IDLE;
                        break;
                }
                break;

            case STATE_CMD_START:
                if (_callback) {
                    _callback->onCallback(CMD_START, (uint8_t*)&ch, 1, _stream);
                }
                _state = STATE_IDLE;
                break;

            case STATE_CMD_END:
                if (_callback) {
                    _callback->onCallback(CMD_END, NULL, 0, _stream);
                }
                _state = STATE_IDLE;
                break;

            case STATE_CMD_MODE:

                if (_callback) {
                    _callback->onCallback(CMD_END, NULL, 0, _stream);
                }
                _state = STATE_IDLE;
                break;
        }
    }
}