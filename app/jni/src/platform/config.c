#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include "config.h"

extern void critical_error(const char *error_title, const char *error_message, ...);

#define MAX_LINE_LENGTH 200

int setting_len = 0;
setting *setting_list = NULL;
boolean dpad_mode = true;

#define add_section(title) { \
    if(count_len){ \
        setting_len ++; \
    }else  if(first_run){ \
        setting * setting_cursor = setting_list + index; \
        strcpy(setting_cursor->name,title); \
        setting_cursor->t = section_; \
        setting_cursor->default_.s = section_no; \
        section_no ++; \
        index++; \
    } \
}

#define add_button(title, b_id, xpos, ypos) { \
    if(count_len){ \
        setting_len ++; \
    }else  if(first_run){ \
        setting * setting_cursor = setting_list + index; \
        strcpy(setting_cursor->name,title); \
        setting_cursor->t = button_; \
        setting_cursor->default_.id = b_id; \
        setting_cursor->xLoc = xpos; \
        setting_cursor->yLoc = ypos; \
        index++; \
    } \
}

long parse_int(const char *name, const char *value, long min, long max) {
    char *endpoint;
    long result = strtol(value, &endpoint, 10);
    if ((result == 0 && endpoint == value) || (*endpoint != '\0')) {
        critical_error("Parsing Error", "Value of '%s' is not a valid integer", name);
    }
    if (result < min || result > max) {
        critical_error("Invalid Value Error", "Value of '%s' is not in the range of %d  and %d",
                       name, min, max);
    }
    return result;
}

double parse_float(const char *name, const char *value, double min, double max) {
    char *endpoint;
    double result = strtof(value, &endpoint);
    if ((result == 0 && endpoint == value) || (*endpoint != '\0')) {
        critical_error("Parsing Error", "Value of '%s' is not a valid decimal", name);
    }
    if (result < min || result > max) {
        critical_error("Invalid Value Error", "Value of '%s' is not in the range of %f  and %f",
                       name, min, max);
    }
    return result;
}

#define parse_val(var_name, name, value, min, max) _Generic((var_name), \
                    int : parse_int, \
                    boolean: parse_int, \
                    double: parse_float)(name,value,min,max)

#define type_val(var) _Generic((var),\
    boolean:boolean_,\
    int: int_,\
    double:double_)

#define set_and_parse_conf(var_name, default, min, max, restart) { \
    if(count_len){ \
        setting_len ++; \
    }else  if(first_run){ \
        var_name = default; \
        setting * setting_cursor = setting_list + index; \
        strcpy(setting_cursor->name,#var_name); \
        setting_cursor->t = type_val(var_name); \
        setting_cursor->need_restart = restart; \
        switch(setting_cursor->t){\
            case(boolean_): \
                setting_cursor->default_.b = (char) default; \
                setting_cursor->min_.b = (char) min; \
                setting_cursor->max_.b = (char) max; \
                break; \
            case(int_): \
                setting_cursor->default_.i = (int) default; \
                setting_cursor->min_.i = (int) min; \
                setting_cursor->max_.i = (int) max; \
                break; \
            case(double_): \
                setting_cursor->default_.d = (double) default; \
                setting_cursor->min_.d = (double) min; \
                setting_cursor->max_.d = (double) max; \
                break; \
            case(section_): \
            case(button_): \
            break; \
        }\
        setting_cursor->value = (void *) &var_name; \
        index++; \
    } \
    else if(strcmp(#var_name,name)==0) { \
        var_name = parse_val(var_name, name, value, min, max); \
        return; \
    } \
}

#define set_and_parse_bool_conf(var_name, default, restart) set_and_parse_conf(var_name,default,false,true,restart)

void set_conf(const char *name, const char *value) {
    static boolean first_run = true;
    static int count_len = true;
    static int section_no = 0;
    int index = 0;
    add_section("Screen Settings");
    set_and_parse_conf(custom_cell_width, 0, 0, INT_MAX, true);
    set_and_parse_conf(custom_cell_height, 0, 0, INT_MAX, true);
    set_and_parse_conf(custom_screen_width, 0, 0, INT_MAX, true);
    set_and_parse_conf(custom_screen_height, 0, 0, INT_MAX, true);
    set_and_parse_bool_conf(force_portrait, false, true);
    set_and_parse_bool_conf(dynamic_colors, true, false);
    set_and_parse_conf(default_graphics_mode,0,0,2,false);
    set_and_parse_bool_conf(tiles_animation, true, false);
    set_and_parse_bool_conf(blend_full_tiles, true, false);
    set_and_parse_conf(filter_mode, 2, 0, 2, true);
    add_section("Input Settings");
    set_and_parse_bool_conf(double_tap_lock, true, false);
    set_and_parse_conf(double_tap_interval, 500, 100, 1e5, false);
    set_and_parse_bool_conf(dpad_enabled, true, false);
    set_and_parse_bool_conf(allow_dpad_mode_change, true, false);
    set_and_parse_conf(dpad_width, 0, 0, INT_MAX, false);
    set_and_parse_conf(dpad_x_pos, 0, 0, INT_MAX, false);
    set_and_parse_conf(dpad_y_pos, 0, 0, INT_MAX, false);
    set_and_parse_bool_conf(default_dpad_mode, true, false);
    set_and_parse_conf(dpad_transparency, 75, 0, 255, false);
    set_and_parse_conf(long_press_interval, 750, 100, 1e5, false);
    set_and_parse_conf(keyboard_visibility, 1, 0, 2, false);
    add_section("Zoom Settings");
    set_and_parse_conf(zoom_mode, 1, 0, 2, false);
    set_and_parse_bool_conf(init_zoom_toggle, false, false);
    set_and_parse_conf(init_zoom, 2.0, 1.0, 10.0, false);
    set_and_parse_conf(max_zoom, 4.0, 1.0, 10.0, false);
    set_and_parse_bool_conf(smart_zoom, true, false);
    set_and_parse_bool_conf(left_panel_smart_zoom, true, false);
    add_section("Update Settings");
    set_and_parse_bool_conf(check_update, true, false);
    set_and_parse_conf(check_update_interval, 1, 0, 1000, false);
    set_and_parse_bool_conf(ask_for_update_check, false, false);
    add_button("Defaults", DEFAULTS_BUTTON_ID, COLS - 8, ROWS - 8);
    add_button("  Cancel", CANCEL_BUTTON_ID, COLS - 8, ROWS - 5);
    add_button("      OK", OK_BUTTON_ID, COLS - 8, ROWS - 2);
    if (!first_run) {
        critical_error("Unknown Configuration",
                       "Configuration '%s' in settings file is not recognized", name);
    } else if (count_len) {
        count_len = false;
        setting_list = malloc(sizeof(setting) * setting_len);
        set_conf(name, value);
    } else {
        first_run = false;
    }
}

void load_conf() {
    FILE *cf;
    if (access(SETTINGS_FILE, F_OK) != -1) {
        cf = fopen(SETTINGS_FILE, "r");
        char line[MAX_LINE_LENGTH];
        while (fgets(line, MAX_LINE_LENGTH, cf) != NULL) {
            boolean empty_line = true; //empty line check
            for (char *c = line; *c && (empty_line = isspace(*c)); c++);
            if (empty_line) { continue; }
            char *name = strtok(line, " ");
            char *value = strtok(NULL, " ");
            value = strtok(value, "\n");
            set_conf(name, value);
        }
        // override custom cell dimensions if custom screen dimensions are present
        if (custom_screen_width) {
            custom_cell_width = custom_screen_width / COLS;
        }
        if (custom_screen_height) {
            custom_cell_height = custom_screen_height / ROWS;
        }
        dpad_mode = default_dpad_mode;
        if (init_zoom > max_zoom) {
            max_zoom = init_zoom;
        }
    } else {
        cf = fopen(SETTINGS_FILE, "w");
    }
    fclose(cf);
}

void save_conf(){
    FILE *st = fopen("../" SETTINGS_FILE, "w");
    for (int i = 0; i < setting_len; i++) {
        setting *s = &setting_list[i];
        switch (s->t) {
            case boolean_:
                if (*((boolean *)s->value) != s->new.b) {
                    restart_game = restart_game || s->need_restart;
                    *((boolean *)s->value) = s->new.b;
                    settings_changed = true;
                }
                if (s->new.b != s->default_.b) {
                    fprintf(st, "%s %s\n", s->name, s->new.b ? "1" : "0");
                }
                break;
            case int_:
                if (*((int *)s->value) != s->new.i) {
                    restart_game = restart_game || s->need_restart;
                    *((int *)s->value) = s->new.i;
                    settings_changed = true;
                }
                if (s->new.i != s->default_.i) {
                    fprintf(st, "%s %d\n", s->name, s->new.i);
                }
                break;
            case double_:
                if (*((double *)s->value) != s->new.d) {
                    restart_game = restart_game || s->need_restart;
                    *((double *)s->value) = s->new.d;
                    settings_changed = true;
                }
                if (s->new.d != s->default_.d) {
                    fprintf(st, "%s %f\n", s->name, s->new.d);
                }
                break;
            case section_:
            case button_:
                break;
        }
    }
    fclose(st);
}