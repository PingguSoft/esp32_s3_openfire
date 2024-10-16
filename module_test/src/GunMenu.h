#ifndef __GUN_MENU_H__
#define __GUN_MENU_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <Wire.h>
#include <arduino-timer.h>

#include <vector>

#include "config.h"
#include "debug.h"

/*
*****************************************************************************************
* GunDisplay
*****************************************************************************************
*/
class GunMenu {
   public:
    typedef enum { TYPE_NORM_STR = 0, TYPE_CENTER_STR, TYPE_BOOL, TYPE_DIGIT_8, TYPE_DIGIT_16 } type_t;
    typedef enum { KEY_NONE = 0, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_BACK } key_t;

    class item_bind {
       public:
        item_bind(uint16_t id, void *data) {
            this->id   = id;
            this->data = data;
        }

        uint16_t get_id() { return this->id; }
        void    *get_data() { return this->data; }

       private:
        uint16_t id;
        void    *data;
    };

    class item_meta {
       public:
        item_meta(int min, int max, void *data = NULL) {
            this->data = data;
            set_range(min, max);
        }
        void set_range(int min, int max) {
            this->min   = min;
            this->max   = max;
            this->digit = 0;
            while (max > 0) {
                max = max / 10;
                this->digit++;
            }
        }
        void    set_data(void *data) { this->data = data; }
        void   *get_data() { return this->data; }
        uint8_t get_digit() { return this->digit; }
        int     get_min() { return this->min; }
        int     get_max() { return this->max; }

       private:
        void   *data;
        int     min;
        int     max;
        uint8_t digit;
    };

    class menu_item {
       public:
        menu_item(uint16_t id, char *name, type_t type, item_meta *meta = NULL, std::vector<menu_item> *child = NULL) {
            this->id    = id;
            this->name  = name;
            this->type  = type;
            this->meta  = meta;
            this->child = child;
        }

        void                    set_name(char *name) { this->name = name; }
        void                    set_meta(item_meta *meta) { this->meta = meta; }
        item_meta              *get_meta() { return this->meta; }
        type_t                  get_type() { return this->type; }
        uint16_t                get_id() { return this->id; }
        char                   *get_name() { return this->name; }
        std::vector<menu_item> *get_child() { return this->child; }

        char *format(char **buf, int sz) {
            switch (type) {
                case GunMenu::TYPE_NORM_STR:
                case GunMenu::TYPE_CENTER_STR:
                    if (child)
                        snprintf(*buf, sz, " %-13s >", name);
                    else
                        snprintf(*buf, sz, " %-13s", name);
                    break;

                case GunMenu::TYPE_DIGIT_8: {
                    uint8_t *v = (uint8_t *)meta->get_data();
                    if (v)
                        snprintf(*buf, sz, " %-13s [%*d]", name, meta->get_digit(), *v);
                    else
                        snprintf(*buf, sz, " %-13s [%*c]", name, meta->get_digit(), 'N');
                } break;

                case GunMenu::TYPE_DIGIT_16: {
                    uint16_t *v = (uint16_t *)meta->get_data();
                    if (v)
                        snprintf(*buf, sz, " %-13s [%*d]", name, meta->get_digit(), *v);
                    else
                        snprintf(*buf, sz, " %-13s [%*c]", name, meta->get_digit(), 'N');
                } break;

                case GunMenu::TYPE_BOOL: {
                    bool *v = (bool *)meta->get_data();
                    if (v)
                        snprintf(*buf, sz, " %-13s [%c]", name, *v ? '*' : ' ');
                    else
                        snprintf(*buf, sz, " %-13s [%c]", name, 'N');
                } break;
            }
            return *buf;
        }

       private:
        uint16_t                id;
        char                   *name;
        type_t                  type;
        item_meta              *meta;
        std::vector<menu_item> *child;
    };

    class Callback {
       public:
        virtual void onMenuItemInit(uint16_t id, GunMenu::menu_item *item) = 0;
        virtual void onMenuItemClicked(uint16_t id, GunMenu::menu_item *item) {}
        virtual void onMenuItemFocused(uint16_t id, GunMenu::menu_item *item) {}
        virtual void onMenuItemLost(uint16_t id, GunMenu::menu_item *item) {}
        virtual void onMenuItemChanged(uint16_t id, GunMenu::menu_item *item) {}
    };

    GunMenu();
    void setup(char *name, std::vector<menu_item> *menu, Callback *callback, std::vector<item_bind> *bind = NULL);
    void handle_menu(key_t key);
    bool updated() {
        bool ret  = _is_dirty;
        _is_dirty = false;
        return ret;
    }
    std::vector<menu_item> *get_list() { return _cur; }
    int8_t                  get_pos() { return _cur_pos; }
    void                    set_pos(int8_t pos) { _cur_pos = pos % _cur->size(); }
    char                   *title();

   private:
    void init(std::vector<menu_item> *menu, std::vector<item_bind> *bind);
    bool add_data(type_t type, item_meta *meta, int val);

    char                                 *_name;
    std::vector<menu_item>               *_top;
    std::vector<menu_item>               *_cur;
    int8_t                                _cur_pos;
    std::vector<std::vector<menu_item> *> _vec_last;
    std::vector<int8_t>                   _vec_last_pos;
    Callback                             *_callback;
    bool                                  _is_dirty;
};

#endif
