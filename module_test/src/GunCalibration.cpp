#include "GunCalibration.h"
#include "debug.h"

typedef struct {
    uint16_t x;
    uint16_t y;
} pos_t;

static const pos_t _mouse_xy[] = {
    {GunHID::mouse_max_x >> 1, GunHID::mouse_max_y >> 1},  // center
    {GunHID::mouse_max_x >> 1, 0},                          // top
    {GunHID::mouse_max_x >> 1, GunHID::mouse_max_y - 1},  // bottom
    {0, GunHID::mouse_max_y >> 1},                          // left
    {GunHID::mouse_max_x - 1, GunHID::mouse_max_y >> 1},  // right
    {GunHID::mouse_max_x >> 1, GunHID::mouse_max_y >> 1},  // center
};

void GunCalibration::setup(GunSettings *settings, GunHID *hid, GunCamera *cam) {
    _hid   = hid;
    _cam   = cam;
    _settings = settings;
    _stage = Cali_Init;
    _timer = new Timer<1, millis, GunCalibration *>();
}

void GunCalibration::begin() {
    _stage    = Cali_Init;
    _ani_info = {0, 0, 0, 0, 0};
    _pd_save  = *_settings->get_profile();
}

void GunCalibration::end() {
    _stage    = Cali_Init;
    _ani_info = {0, 0, 0, 0, 0};
}

bool GunCalibration::timer_mouse_ani_task(GunCalibration *p) {
    int  x;
    int  y;
    bool ret;

    if (p->_ani_info.ctr == 0) {
        x   = p->_ani_info.x;
        y   = p->_ani_info.y;
        ret = false;
    } else {
        x   = p->_hid->get_mouse_x() + p->_ani_info.dx;
        y   = p->_hid->get_mouse_y() + p->_ani_info.dy;
        ret = true;
        p->_ani_info.ctr--;
    }
    p->_hid->report_mouse(x, y, 0);

    return ret;
}

void GunCalibration::mouse_ani_begin(int8_t stage) {
    if (stage <= Cali_Init || stage >= Cali_Verify)
        return;

    uint16_t lx = _mouse_xy[stage - 1].x;
    uint16_t ly = _mouse_xy[stage - 1].y;
    _hid->report_mouse(lx, ly, 0);

    _ani_info.ctr = 30;
    _ani_info.x   = _mouse_xy[stage].x;
    _ani_info.y   = _mouse_xy[stage].y;
    _ani_info.dx  = (_ani_info.x - lx) / _ani_info.ctr;
    _ani_info.dy  = (_ani_info.y - ly) / _ani_info.ctr;
}

bool GunCalibration::loop(uint16_t buttons, GunSettings::GunMode_e mode) {
    bool ret = true;

    _btn_trk.begin(buttons);
    if (mode == GunSettings::GunMode_Calibration) {
        if (!is_mouse_animating()) {
            if (_btn_trk.isPressed(PAD_BUTTON_TR)) {
                GunSettings::profile_data_t* pd = _settings->get_profile();
                OpenFIRE_Layout *layout = _cam->get_layout();

                _stage++;
                LOGV("stage : %d\n", _stage);
                mouse_ani_begin(_stage);
                _timer->every(20, timer_mouse_ani_task, this);

                switch (_stage) {
                    case Cali_Top:
                        pd->topOffset = 0;
                        pd->bottomOffset = 0;
                        pd->leftOffset = 0;
                        pd->rightOffset = 0;

                        pd->adjX = (layout->testMedianX() - (512 << 2)) * cos(layout->Ang()) - (layout->testMedianY() - (384 << 2)) * sin(layout->Ang()) + (512 << 2);
                        pd->adjY = (layout->testMedianX() - (512 << 2)) * sin(layout->Ang()) + (layout->testMedianY() - (384 << 2)) * cos(layout->Ang()) + (384 << 2);

                        if (!pd->irLayout) {
                            // Work out Led locations by assuming height is 100%
                            pd->TLled = (res_x / 2) - ( (layout->W() * (res_y  / layout->H()) ) / 2);
                            pd->TRled = (res_x / 2) + ( (layout->W() * (res_y  / layout->H()) ) / 2);
                        }

                        // Update Cam centre in perspective library
                        _cam->get_perspective()->source(pd->adjX, pd->adjY);
                        _cam->get_perspective()->deinit(0);
                        break;

                    case Cali_Bottom:
                        pd->topOffset = _cam->ry();
                        break;

                    case Cali_Left:
                        pd->bottomOffset = res_y - _cam->ry();
                        break;

                    case Cali_Right:
                        pd->leftOffset = _cam->rx();
                        break;

                    case Cali_Center:
                        pd->rightOffset = res_x - _cam->rx();
                        break;

                    case Cali_Verify:
                        // Apply new Cam center offsets with Offsets applied
                        pd->adjX = (layout->testMedianX() - (512 << 2)) * cos(layout->Ang()) - (layout->testMedianY() - (384 << 2)) * sin(layout->Ang()) + (512 << 2);
                        pd->adjY = (layout->testMedianX() - (512 << 2)) * sin(layout->Ang()) + (layout->testMedianY() - (384 << 2)) * cos(layout->Ang()) + (384 << 2);

                        // Update Cam centre in perspective library
                        _cam->get_perspective()->source(pd->adjX, pd->adjY);
                        _cam->get_perspective()->deinit(0);
                        _settings->set_gun_mode(GunSettings::GunMode_Verification);
                        break;
                }
            } else if (_btn_trk.isPressed(PAD_BUTTON_TL)) {
                _stage = Cali_Init;
                LOGV("Calibration restart !!!\n");
            }
        }
    } else if (mode == GunSettings::GunMode_Verification) {
        _hid->report_mouse(_cam->x(), _cam->y(), 0);

        if (_btn_trk.isPressed(PAD_BUTTON_TR)) {
            end();
            _settings->save();
            LOGV("Calibration done !!!\n");
            ret = false;
        } else if (_btn_trk.isPressed(PAD_BUTTON_TL)) {
            GunSettings::profile_data_t* pd = _settings->get_profile();
            *pd = _pd_save;
            LOGV("Cancel & Retry !!!\n");
            begin();
        }
    }

    _btn_trk.end();
    _timer->tick();
    return ret;
}
