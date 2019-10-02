#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "platform.h"

#define COLOR_MAX UCHAR_MAX
#define MAX_LINE_LENGTH 200
#define LEFT_PANEL_WIDTH 20
#define TOP_LOG_HEIGIHT 3
#define BOTTOM_BUTTONS_HEIGHT 2
#define FRAME_INTERVAL 50
#define ZOOM_CHANGED_INTERVAL 300
#define ZOOM_TOGGLED_INTERVAL 100

typedef struct {
    SDL_Texture *c;
    int width, height, offset_x, offset_y;
} glyph_cache;

struct brogueConsole currentConsole;
extern playerCharacter rogue;
extern creature player;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture * screen_texture;
static SDL_DisplayMode display;
static int cell_w, cell_h;
static boolean screen_changed = false;
static TTF_Font *font;
static glyph_cache font_cache[UCHAR_MAX];
static SDL_Texture * dpad_image_select;
static SDL_Texture * dpad_image_move;
static SDL_Rect dpad_area;
static boolean dpad_mode = true;
static boolean ctrl_pressed = false;
static double zoom_level = 1.0;
static SDL_Rect left_panel_box;
static SDL_Rect log_panel_box;
static SDL_Rect button_panel_box;
static SDL_Rect grid_box;
static SDL_Rect grid_box_zoomed;
static boolean game_started = false;
static rogueEvent current_event;
static boolean current_event_set = false;
static boolean zoom_toggle = false;

//Config Values
static int custom_cell_width = 0;
static int custom_cell_height = 0;
//int custom_screen_width
//int custom_screen_height
static boolean force_portrait = false;
static boolean double_tap_lock = true;
static int double_tap_interval = 500;
static boolean dynamic_colors = 1;
static boolean dpad_enabled = true;
static int dpad_width = 0;
static int dpad_x_pos = 0;
static int dpad_y_pos = 0;
static boolean allow_dpad_mode_change = true;
//boolean default_dpad_mode
static int long_press_interval = 750;
static int dpad_transparency = 75;
static boolean keyboard_always_on = false;
static int zoom_mode = 1;
static double init_zoom = 2.0;
static boolean init_zoom_toggle = false;
static double max_zoom = 4.0;
static boolean smart_zoom = true;
static int filter_mode = 2;

void load_conf(){
    if (access("settings.conf", F_OK) != -1) {
        FILE * cf = fopen("settings.conf","r");
        char line[MAX_LINE_LENGTH];
        int custom_screen_width = 0;
        int custom_screen_height = 0;
        while(fgets(line,MAX_LINE_LENGTH,cf)!=NULL){
            char * name = strtok(line," ");
            char * value = strtok(NULL," ");
            value = strtok(value,"\n");
            if(strcmp("custom_cell_width",name)==0) {
                custom_cell_width = atoi(value);
            }else if(strcmp("custom_cell_height",name)==0) {
                    custom_cell_height = atoi(value);
            }else if(strcmp("custom_screen_width",name)==0) {
                custom_screen_width = atoi(value);
            }else if(strcmp("custom_screen_height",name)==0) {
                    custom_screen_height = atoi(value);
            }else if(strcmp("double_tap_lock",name)==0) {
                double_tap_lock = atoi(value);
            }else if(strcmp("double_tap_interval",name)==0) {
                double_tap_interval = atoi(value);
            }else if(strcmp("dynamic_colors",name)==0){
                dynamic_colors = atoi(value);
            }else if(strcmp("force_portrait",name)==0){
                force_portrait = atoi(value);
            }else if(strcmp("dpad_enabled",name)==0){
                dpad_enabled = atoi(value);
            }else if(strcmp("dpad_width",name)==0){
                dpad_width = atoi(value);
            }else if(strcmp("dpad_x_pos",name)==0){
                dpad_x_pos = atoi(value);
            }else if(strcmp("dpad_y_pos",name)==0){
                dpad_y_pos = atoi(value);
            }else if(strcmp("allow_dpad_mode_change",name)==0){
                allow_dpad_mode_change = atoi(value);
            }else if(strcmp("default_dpad_mode",name)==0){
                dpad_mode = atoi(value);
            }else if(strcmp("dpad_transparency",name)==0){
                dpad_transparency = atoi(value);
            }else if(strcmp("long_press_interval",name)==0){
                long_press_interval = atoi(value);
            }else if(strcmp("keyboard_always_on",name)==0){
                keyboard_always_on = atoi(value);
            }else if(strcmp("zoom_mode",name)==0){
                zoom_mode = atoi(value);
            }else if(strcmp("max_zoom",name)==0){
                max_zoom = atof(value);
            }else if(strcmp("init_zoom",name)==0){
                init_zoom = atof(value);
            }else if(strcmp("init_zoom_toggle",name)==0){
                init_zoom_toggle = atoi(value);
            }else if(strcmp("smart_zoom",name)==0){
                smart_zoom = atoi(value);
            }else if(strcmp("filter_mode",name)==0){
                filter_mode = atoi(value);
            }
        }
        // override custom cell dimensions if custom screen dimensions are present
        if(custom_screen_width){
           custom_cell_width = custom_screen_width / COLS;
        }
        if(custom_screen_height){
           custom_cell_height = custom_screen_height / ROWS;
        }
        fclose(cf);
    }
}

uint8_t convert_color(short c) {
    c = c * COLOR_MAX / 100;
    return max(0,min(c,COLOR_MAX));
}

void draw_glyph(uint16_t c, SDL_Rect rect, uint8_t r, uint8_t g, uint8_t b) {
    if (c <= ' ') { //Empty Cell Optimization
        return;
    }
    uint8_t key;
    if (c < UCHAR_MAX) {
        key = c;
    } else {
        switch (c) {
            case FLOOR_CHAR:
                key = 128 + 0;
                break;
            case CHASM_CHAR:
                key = 128 + 1;
                break;
            case TRAP_CHAR:
                key = 128 + 2;
                break;
            case FIRE_CHAR:
                key = 128 + 3;
                break;
            case FOLIAGE_CHAR:
                key = 128 + 4;
                break;
            case AMULET_CHAR:
                key = 128 + 5;
                break;
            case SCROLL_CHAR:
                key = 128 + 6;
                break;
            case RING_CHAR:
                key = 128 + 7;
                break;
            case WEAPON_CHAR:
                key = 128 + 8;
                break;
            case GEM_CHAR:
                key = 128 + 9;
                break;
            case TOTEM_CHAR:
                key = 128 + 10;
                break;
            case BAD_MAGIC_CHAR:
                key = 128 + 12;
                break;
            case GOOD_MAGIC_CHAR:
                key = 128 + 13;
                break;
            case DOWN_ARROW_CHAR:
                key = 144 + 1;
                break;
            case LEFT_ARROW_CHAR:
                key = 144 + 2;
                break;
            case RIGHT_ARROW_CHAR:
                key = 144 + 3;
                break;
            case UP_TRIANGLE_CHAR:
                key = 144 + 4;
                break;
            case DOWN_TRIANGLE_CHAR:
                key = 144 + 5;
                break;
            case OMEGA_CHAR:
                key = 144 + 6;
                break;
            case THETA_CHAR:
                key = 144 + 7;
                break;
            case LAMDA_CHAR:
                key = 144 + 8;
                break;
            case KOPPA_CHAR:
                key = 144 + 9;
                break;
            case CHARM_CHAR:
                key = 144 + 9;
                break;
            case LOZENGE_CHAR:
                key = 144 + 10;
                break;
            case CROSS_PRODUCT_CHAR:
                key = 144 + 11;
                break;
            default:
                key = '?';
                break;
        }
    }
    glyph_cache *lc = &font_cache[key];
    if (lc->c == NULL) {
        SDL_Color fc = {COLOR_MAX, COLOR_MAX, COLOR_MAX};
        SDL_Surface *text = TTF_RenderGlyph_Blended(font, key, fc);
        lc->width = text->w;
        lc->height = text->h;
        lc->offset_x = (rect.w - text->w) / 2;
        lc->offset_y = (rect.h - text->h) / 2;
        lc->c = SDL_CreateTextureFromSurface(renderer, text);
        SDL_FreeSurface(text);
    }
    SDL_Rect font_rect;
    font_rect.x = rect.x + lc->offset_x;
    font_rect.y = rect.y + lc->offset_y;
    font_rect.w = lc->width;
    font_rect.h = lc->height;
    SDL_SetTextureColorMod(lc->c, r, g, b);
    SDL_RenderCopy(renderer, lc->c, NULL, &font_rect);
}

TTF_Font *init_font_size(char *font_path, int size) {
    TTF_Font *current_font = TTF_OpenFont(font_path, size);
    if (TTF_FontLineSkip(current_font) <= (cell_h - 2)){
        int advance;
        if(TTF_GlyphMetrics(current_font,'a',NULL,NULL,NULL,NULL,&advance) == 0 && advance <= (cell_w -2)){
            return current_font;
        }
    }
    TTF_CloseFont(current_font);
    return NULL;
}

boolean init_font() {
    char font_path[PATH_MAX];
    realpath("custom.ttf", font_path);
    if (access(font_path, F_OK) == -1) {
        strcpy(font_path, "default.ttf");
    }
    int size = 5;
    font = init_font_size(font_path, size);
    if (font == NULL) {
        return false;
    }
    while (true) {
        size++;
        TTF_Font *new_size = init_font_size(font_path, size);
        if (new_size) {
            TTF_CloseFont(font);
            font = new_size;
        } else {
            return true;
        }
    }
}

//Hacky solution to check if a confirmation message or a menu is shown
#define static_string_len(s) (sizeof(s)/sizeof(s[0]) - 2)
boolean overlay_shown_zoom_out(int16_t c,uint8_t x,uint8_t y){
    //write if c > 0, only check otherwise
    static char char_buffer[ROWS][COLS+1] = {0}; //COLS + 1 to use rows as strings

    static char no_message[] = "No";
    static uint8_t no_message_len = static_string_len(no_message);
    static char * no_message_pos = NULL;

    static char yes_message[] = "Yes";
    static uint8_t yes_message_len = static_string_len(yes_message);
    static char * yes_message_pos = NULL;

    static char inventory_message[] = "-- press (a-z) for more info --";
    static uint8_t inventory_message_len = static_string_len(inventory_message);
    static char * inventory_message_pos = NULL;

    static char log_message[] = "--MORE--";
    static uint8_t log_message_len = static_string_len(log_message);
    static char * log_message_pos = NULL;

    static char menu_message[] = "Quit without saving";
    static uint8_t menu_message_len = static_string_len(menu_message);
    static char * menu_message_pos = NULL;

    if(!smart_zoom){
        return false;
    }
    if(c > 0){
        char * new_pos;
        char_buffer[y][x] = c;
        switch(c){
            case '-':
                if(x >= inventory_message_len){
                    new_pos =  &char_buffer[y][x-inventory_message_len];
                    if(strncmp(new_pos,inventory_message,inventory_message_len) == 0){
                        inventory_message_pos = new_pos;
                        return true;
                    }
                }
                if(x >= log_message_len){
                    new_pos =  &char_buffer[y][x-log_message_len];
                    if(strncmp(new_pos,log_message,log_message_len) == 0){
                        log_message_pos = new_pos;
                    }
                }
                break;
            case 's':
                if(x >= yes_message_len + no_message_len){
                    new_pos =  &char_buffer[y][x-yes_message_len];
                    if(strncmp(new_pos,yes_message,yes_message_len) == 0){
                        for(int i = x - yes_message_len;i>=1;i++){
                            if(char_buffer[y][i] == 'o' && char_buffer[y][i-1] == 'N') {
                                yes_message_pos = new_pos;
                                no_message_pos = &char_buffer[y][i-1];
                                return true;

                            }
                        }
                    }
                }
                break;
            case 'g':
                if(x >= menu_message_len){
                    new_pos =  &char_buffer[y][x-menu_message_len];
                    if(strncmp(new_pos,menu_message,menu_message_len) == 0){
                        menu_message_pos = new_pos;
                        return true;
                    }
                }
                break;
        }
    }else{
        if(inventory_message_pos && strncmp(inventory_message_pos,inventory_message,inventory_message_len) == 0){
            return true;
        }
        inventory_message_pos = NULL;
        if(log_message_pos && strncmp(log_message_pos,log_message,log_message_len) == 0){
            return true;
        }
        log_message_pos = NULL;
        if(yes_message_pos && no_message_pos &&
            strncmp(no_message_pos,no_message,no_message_len) == 0 &&
            strncmp(yes_message_pos,yes_message,yes_message_len) == 0){
            return true;
        }
        yes_message_pos = NULL;
        no_message_pos  = NULL;
        if(menu_message_pos && strncmp(menu_message_pos,menu_message,menu_message_len) == 0){
            return true;
        }
        menu_message_pos = NULL;
    }
    return false;
}


void process_events() {
    static int16_t cursor_x = 0;
    static int16_t cursor_y = 0;
    static uint32_t ctrl_time = 0;
    static uint32_t long_press_time = 0;
    static uint32_t prev_click_time = 0;
    static uint32_t zoom_changed_time = 0;
    static uint32_t zoom_toggled_time = 0;
    static boolean virtual_keyboard = false;
    static boolean on_dpad = false;
    current_event.shiftKey = false;
    current_event.controlKey = ctrl_pressed;
    current_event.eventType=EVENT_ERROR;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        float raw_input_x,raw_input_y;
        switch (event.type) {
            case SDL_APP_WILLENTERFOREGROUND:
                screen_changed = true;
            case SDL_FINGERDOWN:
                if(event.tfinger.fingerId !=0){
                    current_event.eventType=EVENT_ERROR;
                    if(event.tfinger.fingerId == 1){
                        zoom_toggled_time = SDL_GetTicks();
                    }
                    break;
                }
                if(!SDL_TICKS_PASSED(SDL_GetTicks(),zoom_changed_time+ZOOM_CHANGED_INTERVAL)){
                    break;
                }
                on_dpad = false;
                raw_input_x = event.tfinger.x * display.w;
                raw_input_y = event.tfinger.y * display.h;
                SDL_Point p = {.x = raw_input_x,.y=raw_input_y};
                if(dpad_enabled){
                    SDL_Point p = {.x = raw_input_x,.y=raw_input_y};
                    if(SDL_PointInRect(&p,&dpad_area)){
                        on_dpad = true;
                        long_press_time = SDL_GetTicks();
                        break;
                    }
                }
                if(zoom_mode != 0 && zoom_level!= 1 && zoom_toggle && SDL_PointInRect(&p,&grid_box)){
                    raw_input_x = (raw_input_x-grid_box.x)/zoom_level + grid_box_zoomed.x;
                    raw_input_y = (raw_input_y-grid_box.y)/zoom_level + grid_box_zoomed.y;
                }
                if(!double_tap_lock || SDL_TICKS_PASSED(SDL_GetTicks(),prev_click_time+double_tap_interval)){
                    cursor_x = min(COLS - 1,raw_input_x / cell_w);
                    cursor_y = min(ROWS -1,raw_input_y / cell_h);
                }
                current_event.param1 = cursor_x;
                current_event.param2 = cursor_y;
                current_event.eventType = MOUSE_DOWN;
                long_press_time = SDL_GetTicks();
                prev_click_time = SDL_GetTicks();
                break;
            case SDL_FINGERUP:
                if(event.tfinger.fingerId !=0){
                    current_event.eventType=EVENT_ERROR;
                    if(event.tfinger.fingerId == 1 && !SDL_TICKS_PASSED(SDL_GetTicks(),zoom_toggled_time+ZOOM_TOGGLED_INTERVAL)){
                        zoom_toggle = !zoom_toggle;
                        screen_changed = true;
                    }
                    break;
                }
                if(!SDL_TICKS_PASSED(SDL_GetTicks(),zoom_changed_time+ZOOM_CHANGED_INTERVAL)){
                    break;
                }
                raw_input_x = event.tfinger.x * display.w;
                raw_input_y = event.tfinger.y * display.h;
                if(dpad_enabled && on_dpad){
                    on_dpad = false;
                    if(long_press_time == 0){
                        break;
                    }
                    SDL_Point p = {.x = raw_input_x,.y=raw_input_y};
                    if(SDL_PointInRect(&p,&dpad_area)){
                        int diff_x = 0,diff_y = 0;
                        SDL_Rect min_x  = {.x = dpad_area.x,.y = dpad_area.y,.w = dpad_area.w /3,.h=dpad_area.h};
                        if(SDL_PointInRect(&p,&min_x)){
                            diff_x = -1;
                        }
                        SDL_Rect max_x  = {.x = dpad_area.x+2*dpad_area.w/3,.y = dpad_area.y,.w = dpad_area.w /3,.h=dpad_area.h};
                        if(SDL_PointInRect(&p,&max_x)){
                            diff_x = 1;
                        }
                        SDL_Rect min_y  = {.x = dpad_area.x,.y = dpad_area.y,.w = dpad_area.w,.h=dpad_area.h/3};
                        if(SDL_PointInRect(&p,&min_y)){
                            diff_y = -1;
                        }
                        SDL_Rect max_y  = {.x = dpad_area.x,.y = dpad_area.y+2*dpad_area.h/3,.w = dpad_area.w,.h=dpad_area.h/3};
                        if(SDL_PointInRect(&p,&max_y)){
                            diff_y = 1;
                        }
                        if(dpad_mode){
                            diff_y *= -1;
                            current_event.eventType = KEYSTROKE;
                            if(diff_x < 0){
                                if(diff_y < 0){
                                    current_event.param1 = DOWNLEFT_KEY;
                                }else if(diff_y > 0){
                                    current_event.param1 = UPLEFT_KEY;
                                }else{
                                    current_event.param1 = LEFT_KEY;
                                }
                            }else if(diff_x > 0){
                                if(diff_y < 0){
                                    current_event.param1 = DOWNRIGHT_KEY;
                                }else if(diff_y > 0){
                                    current_event.param1 = UPRIGHT_KEY;
                                }else{
                                    current_event.param1 = RIGHT_KEY;
                                }
                            }else if (diff_y<0){
                                current_event.param1 = DOWN_KEY;
                            }else if(diff_y>0){
                                current_event.param1 = UP_KEY;
                            }else{
                                current_event.param1 = RETURN_KEY;
                            }
                        }else {
                            cursor_x = max(LEFT_PANEL_WIDTH + 1, min(COLS - 1, cursor_x + diff_x));
                            cursor_y = max(TOP_LOG_HEIGIHT, min(ROWS - (BOTTOM_BUTTONS_HEIGHT + 1), cursor_y + diff_y));
                            current_event.param1 = cursor_x;
                            current_event.param2 = cursor_y;
                            current_event.eventType = MOUSE_ENTERED_CELL;
                            if(!diff_x && !diff_y){
                                current_event.eventType = MOUSE_UP;
                            }
                        }
                        break;
                    }
                }
                virtual_keyboard = false;
                current_event.param1 = cursor_x;
                current_event.param2 = cursor_y;
                if(current_event.param1 < 2){
                    if(current_event.param2 < 2){
                        current_event.eventType = KEYSTROKE;
                        current_event.param1 = ENTER_KEY;

                    }else if(current_event.param2 > (ROWS - 3)){
                        current_event.eventType = KEYSTROKE;
                        current_event.param1 = ESCAPE_KEY;
                    }else{
                        virtual_keyboard = true;
                        SDL_StartTextInput();
                    }
                }else{
                    current_event.eventType = MOUSE_UP;
                }
                if(!(keyboard_always_on || virtual_keyboard)){
                    SDL_StopTextInput();
                }
                break;
            case SDL_FINGERMOTION:
                if(!SDL_TICKS_PASSED(SDL_GetTicks(),zoom_changed_time+ZOOM_CHANGED_INTERVAL)){
                    current_event.eventType = EVENT_ERROR;
                    break;
                }
                if(long_press_time != 0 && SDL_TICKS_PASSED(SDL_GetTicks(),long_press_time + long_press_interval)){
                    if(on_dpad && allow_dpad_mode_change){
                        dpad_mode = !dpad_mode;
                        screen_changed = true;
                        pauseForMilliseconds(0);
                    }
                    long_press_time = 0;
                }

                break;



            case SDL_MULTIGESTURE:
                if(event.mgesture.numFingers==2 && game_started){
                    zoom_level *= (1.0 + event.mgesture.dDist*3);
                    zoom_level = max(1.0,min(zoom_level,max_zoom));
                    zoom_changed_time = SDL_GetTicks();
                    screen_changed = true;
                    if(SDL_TICKS_PASSED(SDL_GetTicks(),zoom_toggled_time+ZOOM_TOGGLED_INTERVAL)) {
                        zoom_toggle = true;
                        zoom_toggled_time = 0;
                    }
                }
                else if(event.mgesture.numFingers==3){
                    if(!ctrl_pressed){
                        current_event.controlKey = ctrl_pressed = true;
                        ctrl_time = SDL_GetTicks();
                    }
                }
                break;
            case SDL_KEYDOWN:
                current_event.eventType = KEYSTROKE;
                SDL_Scancode k = event.key.keysym.sym;
                if (event.key.keysym.mod & KMOD_CTRL){
                    current_event.controlKey = ctrl_pressed = true;
                    ctrl_time = SDL_GetTicks();
                }
                switch(k){
                    case SDLK_AC_BACK:
                    case SDLK_ESCAPE:
                        current_event.param1 = ESCAPE_KEY;
                        break;
                    case SDLK_BACKSPACE:
                    case SDLK_DELETE:
                        current_event.param1 = DELETE_KEY;
                        break;
                    case SDLK_LEFT:
                        current_event.param1 = LEFT_KEY;
                        break;
                    case SDLK_RIGHT:
                        current_event.param1 = RIGHT_KEY;
                        break;
                    case SDLK_UP:
                        current_event.param1 = UP_KEY;
                        break;
                    case SDLK_DOWN:
                        current_event.param1 = DOWN_KEY;
                        break;
                    case SDLK_SPACE:
                        current_event.param1 = ACKNOWLEDGE_KEY;
                        break;
                    case SDLK_RETURN:
                        current_event.param1 = RETURN_KEY;
                        break;
                    case SDLK_TAB:
                        current_event.param1 = TAB_KEY;
                        break;
                    default:
                        if(event.key.keysym.mod & (KMOD_SHIFT | KMOD_CAPS)){
                            if('a' <= k && k <= 'z'){
                                k += 'A' - 'a';
                                current_event.shiftKey = true;
                            }else{
                                k += '?' - '/';
                            }

                        }
                        current_event.param1 = k;
                        break;
                }
                break;
        }
    }
    if(SDL_TICKS_PASSED(SDL_GetTicks(),ctrl_time + 1000)){
        //Wait for 1 second to disable CTRL after pressing and another input
        current_event.controlKey = ctrl_pressed = false;
    }
    static boolean overlay_zoom_out_prev = false;
    static boolean overlay_zoom_out_current = false;
    static toggle_before_overlay = false;
    overlay_zoom_out_prev = overlay_zoom_out_current;
    overlay_zoom_out_current = overlay_shown_zoom_out(-1,0,0);
    if(!overlay_zoom_out_current && overlay_zoom_out_prev){
        zoom_toggle = toggle_before_overlay;
    }else if(overlay_zoom_out_current && overlay_zoom_out_prev){
       zoom_toggle = false;
    }else if(overlay_zoom_out_current && !overlay_zoom_out_prev){
        toggle_before_overlay = zoom_toggle;
        zoom_toggle = false;
    }
    current_event_set = (current_event.eventType != EVENT_ERROR);
}

void TouchScreenGameLoop() {
    load_conf();
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    if(force_portrait){
        SDL_SetHint(SDL_HINT_ORIENTATIONS,"Portrait PortraitUpsideDown");
    }
    char render_hint[2] = {filter_mode+'0',0};
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, render_hint);
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return;
    }
    if (TTF_Init() != 0) {
        SDL_Log("Unable to initialize SDL_ttf: %s", SDL_GetError());
        return;
    }
    if (SDL_GetDesktopDisplayMode(0, &display) != 0) {
        SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        return;
    }
    if (SDL_CreateWindowAndRenderer(display.w, display.h, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALWAYS_ON_TOP, &window,
                                    &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s",
                     SDL_GetError());
        return;
    }
    if(force_portrait){
        int t = display.w;
        display.w = display.h;
        display.h = t;
    }
    if(custom_cell_width != 0){
        cell_w = custom_cell_width;
    }else {
        cell_w = display.w / COLS;
    }
    if(custom_cell_height != 0){
       cell_h = custom_cell_height;
    }else{
        cell_h = display.h / ROWS;
    }
    left_panel_box = (SDL_Rect) {.x = 0, .y = 0, .w = LEFT_PANEL_WIDTH * cell_w, .h = ROWS * cell_h};
    log_panel_box = (SDL_Rect) {.x = LEFT_PANEL_WIDTH * cell_w, .y = 0, .w = (COLS - LEFT_PANEL_WIDTH) * cell_w, .h = TOP_LOG_HEIGIHT * cell_h};
    button_panel_box = (SDL_Rect) {.x = LEFT_PANEL_WIDTH * cell_w, .y = (ROWS - BOTTOM_BUTTONS_HEIGHT)*cell_h, .w = (COLS - LEFT_PANEL_WIDTH) * cell_w, .h = BOTTOM_BUTTONS_HEIGHT * cell_h};
    grid_box = grid_box_zoomed = (SDL_Rect) {.x = LEFT_PANEL_WIDTH * cell_w,.y = TOP_LOG_HEIGIHT * cell_h,.w = (COLS - LEFT_PANEL_WIDTH) * cell_w,.h=(ROWS - TOP_LOG_HEIGIHT - BOTTOM_BUTTONS_HEIGHT)*cell_h};
    memset(font_cache, 0, UCHAR_MAX * sizeof(glyph_cache));
    if (!init_font()) {
        SDL_Log("Cannot fit font into cells");
        return;
    }
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, display.w, display.h);
    SDL_SetRenderTarget(renderer,screen_texture);
    if(dpad_enabled){
        SDL_Surface * dpad_i = SDL_LoadBMP("dpad.bmp");
        dpad_image_select = SDL_CreateTextureFromSurface(renderer,dpad_i);
        SDL_SetTextureAlphaMod(dpad_image_select,dpad_transparency);
        dpad_image_move = SDL_CreateTextureFromSurface(renderer,dpad_i);
        SDL_SetTextureColorMod(dpad_image_move,255,255,155);
        SDL_SetTextureAlphaMod(dpad_image_move,dpad_transparency);
        SDL_FreeSurface(dpad_i);
        int area_width = min(cell_w*(LEFT_PANEL_WIDTH - 4),cell_h*20);
        dpad_area.h=dpad_area.w= (dpad_width)?dpad_width:area_width;
        dpad_area.x =(dpad_x_pos)?dpad_x_pos: 3*cell_w;
        dpad_area.y = (dpad_y_pos)?dpad_y_pos :(display.h - (area_width + 2*cell_h));
    }
    if(keyboard_always_on){
        SDL_StartTextInput();
    }
    rogueMain();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
}

boolean TouchScreenPauseForMilliseconds(short milliseconds){
    uint32_t init_time = SDL_GetTicks();
    if(screen_changed) {
        screen_changed = false;
        SDL_SetRenderTarget(renderer, NULL);
        if(rogue.depthLevel == 0 || rogue.gameHasEnded || rogue.quit || player.currentHP <= 0){
            zoom_level = 1.0;
            game_started = false;
        }else if(!game_started){
           game_started = true;
            zoom_level = init_zoom;
            zoom_toggle = init_zoom_toggle;
        }
        if(zoom_mode == 0 || zoom_level == 1.0 || !zoom_toggle){
            SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
        }else{
            double width = (COLS - LEFT_PANEL_WIDTH) * cell_w / zoom_level;
            double height = (ROWS - TOP_LOG_HEIGIHT - BOTTOM_BUTTONS_HEIGHT)*cell_h/zoom_level;
            int x,y;
            if(zoom_mode == 2 && (rogue.cursorLoc[0] > 0 || rogue.cursorLoc[1] > 0)){
                x = rogue.cursorLoc[0];
                y = rogue.cursorLoc[1];
            }else{
                x = player.xLoc;
                y = player.yLoc;
            }
            int center_x = x*cell_w+left_panel_box.w - width/2;
            int center_y = y*cell_h+log_panel_box.h-height/2;
            center_x = max(left_panel_box.w,min(center_x,left_panel_box.w+grid_box.w - width));
            center_y = max(log_panel_box.h,min(center_y,log_panel_box.h+grid_box.h - height));
            grid_box_zoomed = (SDL_Rect) {.x = center_x,.y = center_y,.w = width,.h= height};
            SDL_RenderCopy(renderer,screen_texture,&left_panel_box,&left_panel_box);
            SDL_RenderCopy(renderer,screen_texture,&log_panel_box,&log_panel_box);
            SDL_RenderCopy(renderer,screen_texture,&button_panel_box,&button_panel_box);
            SDL_RenderCopy(renderer,screen_texture,&grid_box_zoomed,&grid_box);
        }
        if(dpad_enabled){
            SDL_RenderCopy(renderer, dpad_mode?dpad_image_move:dpad_image_select, NULL, &dpad_area);
        }
        SDL_RenderPresent(renderer);
        SDL_SetRenderTarget(renderer, screen_texture);
    }
    uint32_t epoch = SDL_GetTicks() - init_time;
    if(epoch < milliseconds){
        SDL_Delay(milliseconds - epoch);
    }
    process_events();
    return current_event_set;
}

void TouchScreenNextKeyOrMouseEvent(rogueEvent *returnEvent, boolean textInput, boolean colorsDance) {
    while(!current_event_set){
        process_events();
        if (dynamic_colors && colorsDance) {
            shuffleTerrainColors(3, true);
            commitDraws();
        }
        TouchScreenPauseForMilliseconds(FRAME_INTERVAL);
    }
    *returnEvent = current_event;
    current_event_set = false;
}

void TouchScreenPlotChar(uchar ch,
                         short xLoc, short yLoc,
                         short foreRed, short foreGreen, short foreBlue,
                         short backRed, short backGreen, short backBlue) {

    overlay_shown_zoom_out(ch,xLoc,yLoc);
    SDL_Rect rect;
    rect.x = xLoc * cell_w;
    rect.y = yLoc * cell_h;
    rect.w = cell_w;
    rect.h = cell_h;
    SDL_SetRenderDrawColor(renderer, convert_color(backRed), convert_color(backGreen),convert_color(backBlue), COLOR_MAX);
    SDL_RenderFillRect(renderer, &rect);
    draw_glyph(ch, rect, convert_color(foreRed), convert_color(foreGreen), convert_color(foreBlue));
    screen_changed = true;
}

void TouchScreenRemap(const char *input_name, const char *output_name) {
}

boolean TouchScreenModifierHeld(int modifier) {
    //Shift key check is unnecessary since function is not called with modifier == 0 in anywhere
    return modifier == 1 && ctrl_pressed;
}

struct brogueConsole TouchScreenConsole = {
        TouchScreenGameLoop,
        TouchScreenPauseForMilliseconds,
        TouchScreenNextKeyOrMouseEvent,
        TouchScreenPlotChar,
        TouchScreenRemap,
        TouchScreenModifierHeld
};

int main() {
    chdir(SDL_AndroidGetExternalStoragePath());
    currentConsole = TouchScreenConsole;
    rogue.nextGame = NG_NOTHING;
    rogue.nextGamePath[0] = '\0';
    rogue.nextGameSeed = 0;
    currentConsole.gameLoop();
    exit(0); //FIXME returning does not close the app
}
