#include "GunMenu.h"

GunMenu::GunMenu() {
}

void GunMenu::init(std::vector<menu_item> *menu, std::vector<item_bind> *bind) {
    int                     min = 0;
    std::vector<menu_item> *child;

    for (std::vector<menu_item>::iterator m = menu->begin(); m != menu->end(); m++) {
        int max = 0;

        switch (m->get_type()) {
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
            void *data = NULL;
            for (item_bind b : *bind) {
                if (b.get_id() == m->get_id()) {
                    data = b.get_data();
                    break;
                }
            }

            if (!m->get_meta())
                m->set_meta(new item_meta(min, max, data));
            else
                m->get_meta()->set_data(data);
        }

        if (_callback) {
            _callback->onMenuItemInit(m->get_id(), &(*m));
        }

        child = m->get_child();
        if (child) {
            init(child, bind);
        }
    }
}

void GunMenu::setup(char *name, std::vector<menu_item> *menu, Callback *callback, std::vector<item_bind> *bind) {
    _name     = name;
    _cur_pos  = 0;
    _callback = callback;
    _is_dirty = true;

    init(menu, bind);
    _top = menu;
    _cur = menu;
}

bool GunMenu::add_data(type_t type, item_meta *meta, int val) {
    switch (type) {
        case TYPE_NORM_STR:
        case TYPE_CENTER_STR:
            return false;

        case TYPE_BOOL: {
            bool *v = (bool *)meta->get_data();
            *v      = !*v;
        } break;

        case TYPE_DIGIT_8: {
            uint8_t *v = (uint8_t *)meta->get_data();

            if (val > 0 && *v < meta->get_max())
                *v = min(*v + val, meta->get_max());
            else if (val < 0 && *v > meta->get_min())
                *v = max(*v + val, meta->get_min());
        } break;

        case TYPE_DIGIT_16: {
            uint16_t *v = (uint16_t *)meta->get_data();

            if (val > 0 && *v < meta->get_max())
                *v = min(*v + val, meta->get_max());
            else if (val < 0 && *v > meta->get_min())
                *v = max(*v + val, meta->get_min());
        } break;
    }
    return true;
}

void GunMenu::handle_menu(key_t key) {
    bool updated;

    switch (key) {
        case KEY_UP:
            if (_callback)
                _callback->onMenuItemLost(_cur->at(_cur_pos).get_id(), &_cur->at(_cur_pos));
            _cur_pos--;
            if (_cur_pos < 0)
                _cur_pos = _cur_pos + _cur->size();
            if (_callback)
                _callback->onMenuItemFocused(_cur->at(_cur_pos).get_id(), &_cur->at(_cur_pos));
            break;

        case KEY_DOWN:
            if (_callback)
                _callback->onMenuItemLost(_cur->at(_cur_pos).get_id(), &_cur->at(_cur_pos));
            _cur_pos = (_cur_pos + 1) % _cur->size();
            if (_callback)
                _callback->onMenuItemFocused(_cur->at(_cur_pos).get_id(), &_cur->at(_cur_pos));
            break;

        case KEY_RIGHT: {
            menu_item item = _cur->at(_cur_pos);
            updated        = add_data(_cur->at(_cur_pos).get_type(), item.get_meta(), 1);
            if (_callback && updated)
                _callback->onMenuItemChanged(_cur->at(_cur_pos).get_id(), &item);
        } break;

        case KEY_LEFT: {
            menu_item item = _cur->at(_cur_pos);
            updated        = add_data(_cur->at(_cur_pos).get_type(), item.get_meta(), -1);
            if (_callback && updated)
                _callback->onMenuItemChanged(_cur->at(_cur_pos).get_id(), &item);
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
                    _callback->onMenuItemClicked(id, &item);
            } else {
                updated = add_data(_cur->at(_cur_pos).get_type(), item.get_meta(), 1);
                if (_callback && updated)
                    _callback->onMenuItemChanged(id, &item);

                // no changeable item
                if (!updated && _vec_last_pos.size() > 0) {
                    _cur = _vec_last.back();
                    _vec_last.pop_back();
                    _cur_pos = _vec_last_pos.back();
                    _vec_last_pos.pop_back();
                    if (_callback)
                        _callback->onMenuItemClicked(id, &item);
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
    _is_dirty = true;
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
