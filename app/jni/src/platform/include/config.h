#ifndef _config_h_
#define _config_h_

#include "Rogue.h"

#define SETTING_NAME_MAX_LEN 25
#define SETTING_VALUE_MAX_LEN 11
#define SETTINGS_FILE "settings.txt"
#define DEFAULTS_BUTTON_ID 1
#define CANCEL_BUTTON_ID 2
#define OK_BUTTON_ID 3

typedef enum {
    boolean_,
    int_,
    double_,
    section_,
    button_,
} setting_type;

typedef struct {
    char name[SETTING_NAME_MAX_LEN];
    setting_type t;
    union {
        boolean b;
        int i;
        double d;
        short s; //section_number
        short id; //buton id
    } new, default_, min_, max_;
    short xLoc, yLoc;
    void *value;
    boolean need_restart;
} setting;

//Config Values
double custom_cell_width;
double custom_cell_height;
int custom_screen_width;
int custom_screen_height;
boolean force_portrait;
boolean double_tap_lock;
int double_tap_interval;
boolean dynamic_colors;
boolean dpad_enabled;
int dpad_width;
int dpad_x_pos;
int dpad_y_pos;
boolean allow_dpad_mode_change;
boolean default_dpad_mode;
int long_press_interval;
int dpad_transparency;
int keyboard_visibility;
int zoom_mode;
double init_zoom;
boolean init_zoom_toggle;
double max_zoom;
boolean smart_zoom;
boolean left_panel_smart_zoom;
int filter_mode;
boolean check_update;
int check_update_interval;
boolean ask_for_update_check;
int default_graphics_mode;
boolean tiles_animation;
boolean blend_full_tiles;

extern boolean dpad_mode;
extern int setting_len;
extern setting *setting_list;
extern boolean restart_game;
extern boolean settings_changed;

void set_conf(const char *name, const char *value);

void load_conf();

void settings_menu();

#endif
