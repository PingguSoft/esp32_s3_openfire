
#include "GunMameHooker.h"

void GunMameHooker::process() {
    int  ch;
    int  len = _stream->available();
    bool is_read_out = true;

    for (int i = 0; i < len; i++) {
        ch = _stream->peek();
        // debug_printf("%2X [%c]\n", ch, ch);

        switch (_state) {
            case STATE_IDLE:
                switch (ch) {
                    case 'S':
                        _state = STATE_CMD_START;
                        break;

                    case 'E':
                        if (_callback) {
                            _callback->onMameHookCallback(CMD_END, NULL, 0, _stream);
                        }
                        _state = STATE_IDLE;
                        break;

                    case 'M':
                        _pos = 0;
                        _state = STATE_CMD_MODE;
                        break;

                    case 'F':
                        _pos = 0;
                        _state = STATE_CMD_FFB;
                        break;

                    default:
                        _state = STATE_IDLE;
                        break;
                }
                break;

            case STATE_CMD_START:
                if (_callback) {
                    _callback->onMameHookCallback(CMD_START, (uint8_t*)&ch, 1, _stream);
                }
                _state = STATE_IDLE;
                break;

            case STATE_CMD_MODE:
                _buf[_pos++] = ch;
                if (_pos == 3) {
                    if ((_buf[0] == '0' && _buf[2] == '1') || (_buf[0]=='D' && _buf[2] == '3')) {
                        _state = STATE_CMD_MODE_EXT;
                    } else if (_callback) {
                        _callback->onMameHookCallback(CMD_MODE, _buf, 3, _stream);
                        _state = STATE_IDLE;
                    }
                }
                break;

            case STATE_CMD_MODE_EXT:
                _buf[_pos++] = ch;
                if (_callback) {
                    _callback->onMameHookCallback(CMD_MODE, _buf, 4, _stream);
                }
                _state = STATE_IDLE;
                break;

            case STATE_CMD_FFB:
                _buf[_pos++] = ch;
                if (_pos == 2) {
                    if (_buf[0] == 'D') {
                        _state = STATE_CMD_FFB_PARAM;
                    }
                } else if (_pos == 3) {
                    if (('0' <= _buf[0] && _buf[0] <= '1' && _buf[2] == '2') ||
                        ('2' <= _buf[0] && _buf[0] <= '4' && _buf[2] >= '1')) {
                        _state = STATE_CMD_FFB_PARAM;
                    } else if (_callback) {
                        _callback->onMameHookCallback(CMD_FFB, _buf, 3, _stream);
                        _state = STATE_IDLE;
                    }
                }
                break;

            case STATE_CMD_FFB_PARAM:
                if (ch == 'x') {
                    _dec_pos = 0;
                    _buf[_pos++] = ch;
                    _state = STATE_CMD_FFB_PARAM_DECI;
                } else {
                    _state = STATE_IDLE;
                }
                break;

            case STATE_CMD_FFB_PARAM_DECI:
                if ('0' <= ch <= '9') {
                    _dec_pos++;
                    _buf[_pos++] = ch;
                } else {
                    _dec_pos = 3;
                    is_read_out = false;
                }

                if (_dec_pos >= 3) {
                    if (_callback) {
                        _callback->onMameHookCallback(CMD_FFB, _buf, _pos, _stream);
                    }
                    _state = STATE_IDLE;
                }
                break;
        }
        if (is_read_out) {
            _stream->read();
        }
    }
}