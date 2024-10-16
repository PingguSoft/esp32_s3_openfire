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
    typedef enum { TYPE_NONE = 0, TYPE_BOOL, TYPE_DIGIT_8, TYPE_DIGIT_16 } type_t;
    typedef enum { KEY_NONE = 0, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_BACK } key_t;

    class item_meta {
       public:
        item_meta(void *data, int min, int max) {
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
        menu_item(uint16_t id, char *name, type_t type, std::vector<menu_item> *child = NULL, item_meta *meta = NULL) {
            this->id    = id;
            this->name  = name;
            this->type  = type;
            this->child = child;
            this->meta  = meta;
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
                case GunMenu::TYPE_NONE:
                    if (child)
                        snprintf(*buf, sz, " %-14s >", name);
                    else
                        snprintf(*buf, sz, " %-14s", name);
                    break;

                case GunMenu::TYPE_DIGIT_8: {
                    uint8_t *v = (uint8_t *)meta->get_data();
                    snprintf(*buf, sz, " %-14s [%*d]", name, meta->get_digit(), *v);
                } break;

                case GunMenu::TYPE_DIGIT_16: {
                    uint16_t *v = (uint16_t *)meta->get_data();
                    snprintf(*buf, sz, " %-14s [%*d]", name, meta->get_digit(), *v);
                } break;

                case GunMenu::TYPE_BOOL: {
                    bool *v = (bool *)meta->get_data();
                    snprintf(*buf, sz, " %-14s [%c]", name, *v ? '*' : ' ');
                } break;
            }
            return *buf;
        }

       private:
        uint16_t                id;
        char                   *name;
        type_t                  type;
        std::vector<menu_item> *child;
        item_meta              *meta;
    };

    class Callback {
       public:
        typedef enum { ON_INIT = 0, ON_SEL, ON_DESEL, ON_VAL_CHANGE, ON_CLICK } op_t;
        virtual void onMenuCallback(uint16_t id, op_t op, GunMenu::menu_item *item) = 0;
    };

    GunMenu();
    void                    setup(char *name, std::vector<menu_item> *menu, Callback *callback);
    void                    handle_menu(key_t key);
    std::vector<menu_item> *get_list() { return _cur; }
    int8_t                  get_pos() { return _cur_pos; }
    void                    set_pos(int8_t pos) { _cur_pos = pos; }
    char                   *title();

   private:
    void init(std::vector<menu_item> *menu);
    bool add_data(type_t type, item_meta *meta, int val);

    char                                 *_name;
    std::vector<menu_item>               *_top;
    std::vector<menu_item>               *_cur;
    int8_t                                _cur_pos;
    std::vector<std::vector<menu_item> *> _vec_last;
    std::vector<int8_t>                   _vec_last_pos;
    Callback                             *_callback;
};

#endif
