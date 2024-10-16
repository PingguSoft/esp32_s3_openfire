#include "GunMenu.h"

GunMenu::GunMenu() {
}

void GunMenu::init(std::vector<menu_item> *menu) {
    int                     min = 0;
    item_meta              *meta;
    std::vector<menu_item> *child;

    for (menu_item m : *menu) {
        int max = 0;

        switch (m.get_type()) {
            case TYPE_BOOL:
                max = 1;
                break;

            case TYPE_DIGIT_8:
                max = 255;
                break;

            case TYPE_DIGIT_16:
                max = 65535;
                break;
        }

        if (max != 0) {
            meta = new item_meta(NULL, min, max);
            m.set_meta(meta);
        } else {
            meta = NULL;
        }
        if (_callback) {
            _callback->onMenuCallback(m.get_id(), Callback::ON_INIT, &m);
        }

        child = m.get_child();
        if (child) {
            init(child);
        }
    }
}

void GunMenu::setup(char *name, std::vector<menu_item> *menu, Callback *callback) {
    _name     = name;
    _top      = menu;
    _cur      = menu;
    _cur_pos  = 0;
    _callback = callback;
    init(menu);
}

bool GunMenu::add_data(type_t type, item_meta *meta, int val) {
    if (type == TYPE_NONE)
        return false;

    if (type == TYPE_BOOL) {
        bool *v = (bool *)meta->get_data();
        *v      = !*v;
    } else if (type == TYPE_DIGIT_8) {
        uint8_t *v = (uint8_t *)meta->get_data();

        if (val > 0 && *v < meta->get_max())
            *v = min(*v + val, meta->get_max());
        else if (val < 0 && *v > meta->get_min())
            *v = min(*v + val, meta->get_min());
    } else if (type == TYPE_DIGIT_16) {
        uint16_t *v = (uint16_t *)meta->get_data();

        if (val > 0 && *v < meta->get_max())
            *v = min(*v + val, meta->get_max());
        else if (val < 0 && *v > meta->get_min())
            *v = min(*v + val, meta->get_min());
    }
    return true;
}

void GunMenu::handle_menu(key_t key) {
    bool updated;

    switch (key) {
        case KEY_UP:
            if (_callback)
                _callback->onMenuCallback(_cur->at(_cur_pos).get_id(), Callback::ON_DESEL, &_cur->at(_cur_pos));
            _cur_pos--;
            if (_cur_pos < 0)
                _cur_pos = _cur_pos + _cur->size();
            if (_callback)
                _callback->onMenuCallback(_cur->at(_cur_pos).get_id(), Callback::ON_SEL, &_cur->at(_cur_pos));
            break;

        case KEY_DOWN:
            if (_callback)
                _callback->onMenuCallback(_cur->at(_cur_pos).get_id(), Callback::ON_DESEL, &_cur->at(_cur_pos));
            _cur_pos = (_cur_pos + 1) % _cur->size();
            if (_callback)
                _callback->onMenuCallback(_cur->at(_cur_pos).get_id(), Callback::ON_SEL, &_cur->at(_cur_pos));
            break;

        case KEY_RIGHT: {
            menu_item item = _cur->at(_cur_pos);
            updated        = add_data(_cur->at(_cur_pos).get_type(), item.get_meta(), 1);
            if (_callback && updated)
                _callback->onMenuCallback(_cur->at(_cur_pos).get_id(), Callback::ON_VAL_CHANGE, &item);
        } break;

        case KEY_LEFT: {
            menu_item item = _cur->at(_cur_pos);
            updated        = add_data(_cur->at(_cur_pos).get_type(), item.get_meta(), -1);
            if (_callback && updated)
                _callback->onMenuCallback(_cur->at(_cur_pos).get_id(), Callback::ON_VAL_CHANGE, &item);
        } break;

        case KEY_ENTER: {
            menu_item               item  = _cur->at(_cur_pos);
            uint16_t                id    = item.get_id();
            std::vector<menu_item> *child = item.get_child();

            if (child) {
                _vec_last.push_back(_cur);
                _vec_last_pos.push_back(_cur_pos);
                _cur     = child;
                _cur_pos = 0;
                if (_callback)
                    _callback->onMenuCallback(id, Callback::ON_CLICK, &item);
            } else {
                updated = add_data(_cur->at(_cur_pos).get_type(), item.get_meta(), 1);
                if (_callback && updated)
                    _callback->onMenuCallback(id, Callback::ON_VAL_CHANGE, &item);

                // no changeable item
                if (!updated && _vec_last_pos.size() > 0) {
                    _cur = _vec_last.back();
                    _vec_last.pop_back();
                    _cur_pos = _vec_last_pos.back();
                    _vec_last_pos.pop_back();
                    if (_callback)
                        _callback->onMenuCallback(id, Callback::ON_CLICK, &item);
                }
            }
        } break;

        case KEY_BACK:
            if (_vec_last_pos.size() > 0) {
                _cur = _vec_last.back();
                _vec_last.pop_back();
                _cur_pos = _vec_last_pos.back();
                _vec_last_pos.pop_back();
            }
            break;
    }
}

char *GunMenu::title() {
    if (_vec_last_pos.size() > 0) {
        std::vector<menu_item> *last = _vec_last.back();
        int8_t                  p    = _vec_last_pos.back();
        return last->at(p).get_name();
    } else {
        return _name;
    }
}
