#include "GunMenu.h"

GunMenu::GunMenu() {
}

void GunMenu::init(std::vector<menu_item> *menu, std::map<uint16_t, item_meta*> *bind) {
    int                     min = 0;
    int                     pos = 0;
    std::vector<menu_item> *child;

    for (auto m = menu->begin(); m != menu->end(); m++) {   // std::vector<menu_item>::iterator
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
            item_meta *meta;

            // bind first
            if (bind) {
                auto it = bind->find(m->get_id());              // std::map<uint16_t, item_meta*>::iterator
                meta = (it == bind->end()) ? NULL : it->second;
            }
            if (meta == NULL) {
                meta = m->get_meta();
                if (meta == NULL)
                    meta = new item_meta(min, max);
            }
            m->set_meta(meta);
        }

        if (_callback) {
            _callback->onMenuItemInit(m->get_id(), &(*m));
        }

        child = m->get_child();
        if (child) {
            _parent_map.insert(std::make_pair(child, new pinfo_t(menu, pos)));
            init(child, bind);
        }
        pos++;
    }
}

void GunMenu::setup(char *name, std::vector<menu_item> *menu, Callback *callback, std::map<uint16_t, item_meta*> *bind) {
    _name     = name;
    _cur_pos  = 0;
    _callback = callback;
    _is_dirty = true;

    init(menu, bind);
    _top = menu;
    _cur = menu;
}

bool GunMenu::touch(type_t type, item_meta *meta, bool inc) {
    int val = meta->get_step();

    if (!inc)
        val = -val;

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

void GunMenu::handle_event(key_t key) {
    bool is_updated;
    bool is_back = false;

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
            is_updated        = touch(_cur->at(_cur_pos).get_type(), item.get_meta(), true);
            if (_callback && is_updated)
                _callback->onMenuItemChanged(_cur->at(_cur_pos).get_id(), &item);
        } break;

        case KEY_LEFT: {
            menu_item item = _cur->at(_cur_pos);
            is_updated        = touch(_cur->at(_cur_pos).get_type(), item.get_meta(), false);
            if (_callback && is_updated)
                _callback->onMenuItemChanged(_cur->at(_cur_pos).get_id(), &item);
        } break;

        case KEY_ENTER: {
            menu_item               item  = _cur->at(_cur_pos);
            uint16_t                id    = item.get_id();
            std::vector<menu_item> *child = item.get_child();

            if (child) {
                _cur     = child;
                _cur_pos = 0;
                if (_callback)
                    _callback->onMenuItemClicked(id, &item);
            } else {
                is_updated = touch(_cur->at(_cur_pos).get_type(), item.get_meta(), true);
                if (is_updated) {
                    if (_callback)
                        _callback->onMenuItemChanged(id, &item);
                } else {
                    if (_callback)
                        _callback->onMenuItemClicked(id, &item);
                    is_back = true;
                }
            }
        } break;

        case KEY_BACK:
            is_back = true;
            break;
    }

    if (is_back) {
        auto it = _parent_map.find(_cur);
        pinfo_t *p = (it == _parent_map.end()) ? NULL : it->second;
        _cur = p->parent;
        _cur_pos = p->pos;
    }
    _is_dirty = true;
}

char *GunMenu::title() {
    if (_cur == _top)
        return _name;

    auto it = _parent_map.find(_cur);
    pinfo_t *p = (it == _parent_map.end()) ? NULL : it->second;
    return p ? p->parent->at(p->pos).get_name() : _name;
}
