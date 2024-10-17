#ifndef __GUN_MENU_H__
#define __GUN_MENU_H__

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <Wire.h>
#include <arduino-timer.h>

#include <map>
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

    class item_meta {
       public:
        item_meta(int min = 0, int max = 0, uint8_t digit = 1, int8_t step = 1) {
            this->data = NULL;
            this->step = step;
            set_range(min, max, digit);
        }
        item_meta(void *data, int min = 0, int max = 0, uint8_t digit = 1, int8_t step = 1) {
            this->data = data;
            this->step = step;
            set_range(min, max, digit);
        }
        void set_range(int min, int max, uint8_t digit) {
            this->min   = min;
            this->max   = max;
            this->digit = digit;
        }
        void    set_data(void *data) { this->data = data; }
        void   *get_data() { return this->data; }
        uint8_t get_digit() { return this->digit; }
        int     get_min() { return this->min; }
        int     get_max() { return this->max; }
        int8_t  get_step() { return this->step; }

       private:
        void   *data;
        int     min;
        int     max;
        uint8_t digit;
        int8_t  step;
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

    class menu_list {
       private:
        std::vector<menu_item> *parent;
        std::vector<menu_item> *list;
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
    void  setup(char *name, std::vector<menu_item> *menu, Callback *callback = NULL,
                std::map<uint16_t, item_meta *> *bind = NULL);
    void  handle_event(key_t key);
    char *title();

    void                    update() { _is_dirty = true; }
    std::vector<menu_item> *get_list() { return _cur; }
    int8_t                  get_pos() { return _cur_pos; }
    void                    set_pos(int8_t pos) { _cur_pos = pos % _cur->size(); }
    bool                    updated() {
        bool ret  = _is_dirty;
        _is_dirty = false;
        return ret;
    }

   private:
    typedef struct pinfo {
        std::vector<menu_item> *parent;
        int8_t                  pos;

        pinfo(std::vector<menu_item> *parent, int8_t pos) {
            this->parent = parent;
            this->pos    = pos;
        }
    } pinfo_t;

    void init(std::vector<menu_item> *menu, std::map<uint16_t, item_meta *> *bind);
    bool touch(type_t type, item_meta *meta, bool inc);

    char                                         *_name;
    std::vector<menu_item>                       *_top;
    std::vector<menu_item>                       *_cur;
    int8_t                                        _cur_pos;
    std::map<std::vector<menu_item> *, pinfo_t *> _parent_map;
    Callback                                     *_callback;
    bool                                          _is_dirty;
};

#endif
