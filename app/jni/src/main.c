#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <jni.h>
#include <sys/stat.h>
#include <time.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "platform.h"
#include "IncludeGlobals.h"
#include "config.h"

#define COLOR_MAX UCHAR_MAX
#define MAX_ERROR_LENGTH 200
#define LEFT_PANEL_WIDTH 20
#define LEFT_EDGE_WIDTH 2
#define TOP_LOG_HEIGIHT 3
#define BOTTOM_BUTTONS_HEIGHT 2
#define FRAME_INTERVAL 50
#define ZOOM_CHANGED_INTERVAL 300
#define ZOOM_TOGGLED_INTERVAL 100
#define DAY_TO_TIMESTAMP 86400
#define TILES_LEN 256
#define MAX_GLYPH_NO (TILES_LEN * 3)
#define TILES_FLIP_TIME 900
#define MIN_TILE G_UP_ARROW
#define FONT_BOUND_CHAR 139

typedef struct {
    double width, height, offset_x, offset_y;
    SDL_Texture *c;
    boolean animated;
    boolean full_tile;
} glyph_cache;

typedef enum{
   set_false,
   set_true,
   unset,
} bool_store;


struct brogueConsole currentConsole;
extern playerCharacter rogue;
extern creature player;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture * screen_texture;
static SDL_Rect screen;
static SDL_Rect display;
static double cell_w, cell_h;
static boolean screen_changed = false;
static _Atomic boolean resumed = false;
static TTF_Font *font;
static glyph_cache font_cache[MAX_GLYPH_NO];
static uint16_t glyph_index_table[TILES_LEN][3]; //ASCII, tile frame 1, tile frame 2
static int font_width;
static int font_height;
static SDL_Texture * dpad_image_select;
static SDL_Texture * dpad_image_move;
static SDL_Rect dpad_area;
static SDL_Texture * settings_image;
static SDL_Rect settings_icon_area;
static boolean ctrl_pressed = false;
static double zoom_level = 1.0;
static SDL_Rect left_panel_box;
static SDL_Rect log_panel_box;
static SDL_Rect button_panel_box;
static SDL_Rect grid_box;
static SDL_Rect grid_box_zoomed;
static boolean game_started = false;
static rogueEvent current_event;
static boolean zoom_toggle = false;
static boolean restart_game = false;
static boolean settings_changed = false;
static boolean tiles_flipped = false;
static boolean requires_text_input = false;
static boolean virtual_keyboard_active = false;

boolean hasGraphics = true;
boolean graphicsEnabled = true;

void destroy_assets(){
    SDL_SetRenderTarget(renderer,NULL);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void critical_error(const char* error_title,const char* error_message,...){
    char buffer[MAX_ERROR_LENGTH];
    va_list a;
    va_start(a,error_message);
    vsnprintf(buffer, MAX_ERROR_LENGTH,error_message,a);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                             error_title,
                             buffer,
                             NULL);
    va_end(a);
    destroy_assets();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    exit(-1);
}

void start_text_input(){
    if(keyboard_visibility){
        virtual_keyboard_active = true;
        SDL_StartTextInput();
    }
}

void stop_text_input(){
    if(keyboard_visibility != 2){
        virtual_keyboard_active = false;
        SDL_StopTextInput();
    }
}

void create_assets(){
    if (SDL_CreateWindowAndRenderer(display.w, display.h, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALWAYS_ON_TOP, &window,
                                    &renderer)) {
        critical_error("SDL Error", "Couldn't create window and renderer: %s", SDL_GetError());
    }
    memset(font_cache, 0, MAX_GLYPH_NO * sizeof(glyph_cache));
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, display.w, display.h);
    SDL_SetRenderTarget(renderer,screen_texture);
    SDL_SetRenderDrawColor(renderer,0,0,0,COLOR_MAX);
    SDL_RenderClear(renderer);
    if(dpad_enabled){
        SDL_Surface * dpad_i = SDL_LoadBMP("dpad.bmp");
        dpad_image_select = SDL_CreateTextureFromSurface(renderer,dpad_i);
        SDL_SetTextureAlphaMod(dpad_image_select,dpad_transparency);
        dpad_image_move = SDL_CreateTextureFromSurface(renderer,dpad_i);
        SDL_SetTextureColorMod(dpad_image_move,COLOR_MAX,COLOR_MAX,155);
        SDL_SetTextureAlphaMod(dpad_image_move,dpad_transparency);
        SDL_FreeSurface(dpad_i);
        double area_width = min(cell_w*(LEFT_PANEL_WIDTH - 4),cell_h*20);
        dpad_area.h = dpad_area.w =(dpad_width)?dpad_width:area_width;
        dpad_area.x = (dpad_x_pos)?dpad_x_pos: 3*cell_w;
        dpad_area.y = (dpad_y_pos)?dpad_y_pos :(display.h - (area_width + 2*cell_h));
    }
    SDL_Surface *settings_surface = SDL_LoadBMP("settings.bmp");
    settings_image = SDL_CreateTextureFromSurface(renderer, settings_surface);
    SDL_SetTextureAlphaMod(settings_image, COLOR_MAX/3);
    SDL_FreeSurface(settings_surface);
    settings_icon_area.h = settings_icon_area.w = min(cell_w*(LEFT_PANEL_WIDTH - 4),cell_h*20);
    settings_icon_area.x = 2*cell_w;
    settings_icon_area.y = (display.h - (settings_icon_area.h + 2*cell_h));

    if(keyboard_visibility == 2){
        start_text_input();
    }
}

uint8_t convert_color(short c) {
    c = c * COLOR_MAX / 100;
    return max(0,min(c,COLOR_MAX));
}

boolean smart_zoom_allowed(){
    if(!smart_zoom){
        return true;
    }
    return !(gameStat.titleMenuShown || gameStat.fileDialogShown ||
        gameStat.messageArchiveShown || gameStat.inventoryShown ||
        gameStat.menuShown || gameStat.confirmShown);
}

boolean is_zoomed(){
    return zoom_mode != 0 && zoom_level != 1.0 && zoom_toggle && smart_zoom_allowed();
}

int suspend_resume_filter(void *userdata, SDL_Event *event){
    switch(event->type){
        case SDL_APP_WILLENTERBACKGROUND:
            return 0;
        case SDL_APP_WILLENTERFOREGROUND:
            resumed = true;
            return 0;

    }
    return 1;
}

void draw_glyph(enum displayGlyph c, SDL_FRect rect, uint8_t r, uint8_t g, uint8_t b) {
    if (c <= ' ') { //Empty Cell Optimization
        return;
    }
    int mode = graphicsEnabled*(1+tiles_flipped);
    uint16_t key;
    if (c < MIN_TILE) {
        key = c;
    } else {
        key = glyph_index_table[c - MIN_TILE][mode];
    }
    glyph_cache *lc = &font_cache[key];
    if (lc->c == NULL) {
        SDL_Color fc = {COLOR_MAX, COLOR_MAX, COLOR_MAX};
        SDL_Surface *text;
        if((key >= 2*TILES_LEN) && !TTF_GlyphIsProvided(font,key)){
            //fallback to first frame
            glyph_index_table[c - MIN_TILE][mode] -= TILES_LEN;
            draw_glyph(c,rect,r,g,b);
            return;
        }else{
            font_cache[c].animated = true;
        }
        text = TTF_RenderGlyph_Blended(font, key, fc);
        int minx,maxx,miny,maxy,fw,fh;
        TTF_GlyphMetrics(font,key,&minx,&maxx,&miny,&maxy,NULL);
        fw = maxx - minx;
        fh = maxy - miny;
        if((fw >= font_width) && (fh >= font_height)){
            lc->full_tile = true;
        }
        lc->offset_x = (rect.w - text->w) / 2;
        lc->offset_y = (rect.h - text->h) / 2;
        lc->width = text->w;
        lc->height = text->h;
        lc->c = SDL_CreateTextureFromSurface(renderer, text);
        SDL_FreeSurface(text);
    }

    SDL_SetTextureColorMod(lc->c, r, g, b);
    if(blend_full_tiles && (lc->full_tile)){
        SDL_Rect src = {.x = 0,.y=0,.w=font_width,.h=font_height};
        SDL_RenderCopyF(renderer, lc->c, &src, &rect);
    }else{
        SDL_Rect font_rect;
        font_rect.x = rect.x + lc->offset_x;
        font_rect.y = rect.y + lc->offset_y;
        font_rect.w =  lc->width;
        font_rect.h = lc->height;
        SDL_RenderCopy(renderer, lc->c, NULL, &font_rect);
    }

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
#define init_glyph_index(glyph,ascii,tile) \
    glyph_index_table[glyph-MIN_TILE][0] = ascii; \
    glyph_index_table[glyph-MIN_TILE][1] = tile; \
    glyph_index_table[glyph-MIN_TILE][2] = tile + TILES_LEN;

void init_glyph_index_table(){
    init_glyph_index(G_UP_ARROW, 128 + 8, 128 + 8)
    init_glyph_index(G_DOWN_ARROW, 144 + 1, 144 + 1)
    init_glyph_index(G_PLAYER, '@', 256)
    init_glyph_index(G_EASY, '&', 257)
    init_glyph_index(G_FOOD, ';', 258)
    init_glyph_index(G_GOLD, '*', 259)
    init_glyph_index(G_ARMOR, '[', 260)
    init_glyph_index(G_WEAPON, 128 + 8, 261)
    init_glyph_index(G_POTION, '!', 262)
    init_glyph_index(G_SCROLL, 128 + 6, 263)
    init_glyph_index(G_RING, 128 + 7, 264)
    init_glyph_index(G_STAFF, '\\', 265)
    init_glyph_index(G_WAND, '~', 266)
    init_glyph_index(G_CHARM, 144 + 9, 267)
    init_glyph_index(G_ANCIENT_SPIRIT, 'M', 268)
    init_glyph_index(G_BAT, 'v', 269)
    init_glyph_index(G_BLOAT, 'b', 270)
    init_glyph_index(G_BOG_MONSTER, 'B', 271)
    init_glyph_index(G_CENTAUR, 'C', 272)
    init_glyph_index(G_CENTIPEDE, 'c', 273)
    init_glyph_index(G_DAR_BATTLEMAGE, 'd', 274)
    init_glyph_index(G_DAR_BLADEMASTER, 'd', 275)
    init_glyph_index(G_DAR_PRIESTESS, 'd', 276)
    init_glyph_index(G_DRAGON, 'D', 277)
    init_glyph_index(G_EEL, 'e', 278)
    init_glyph_index(G_EGG, 128 + 9, 279)
    init_glyph_index(G_FLAMEDANCER, 'F', 280)
    init_glyph_index(G_FURY, 'f', 281)
    init_glyph_index(G_GOBLIN, 'g', 282)
    init_glyph_index(G_GOBLIN_CHIEFTAN, 'g', 283)
    init_glyph_index(G_GOBLIN_MAGIC, 'g', 284)
    init_glyph_index(G_GOLEM, 'G', 285)
    init_glyph_index(G_GUARDIAN, 223, 286)
    init_glyph_index(G_IFRIT, 'I', 287)
    init_glyph_index(G_IMP, 'i', 288)
    init_glyph_index(G_JACKAL, 'j', 289)
    init_glyph_index(G_JELLY, 'J', 290)
    init_glyph_index(G_KOBOLD, 'k', 291)
    init_glyph_index(G_KRAKEN, 'K', 292)
    init_glyph_index(G_LICH, 'L', 293)
    init_glyph_index(G_MONKEY, 'm', 294)
    init_glyph_index(G_MOUND, 'a', 295)
    init_glyph_index(G_NAGA, 'N', 296)
    init_glyph_index(G_OGRE, 'O', 297)
    init_glyph_index(G_OGRE_MAGIC, 'O', 298)
    init_glyph_index(G_PHANTOM, 'P', 299)
    init_glyph_index(G_PHOENIX, 'P', 300)
    init_glyph_index(G_PIXIE, 'p', 301)
    init_glyph_index(G_RAT, 'r', 302)
    init_glyph_index(G_REVENANT, 'R', 303)
    init_glyph_index(G_SALAMANDER, 'S', 304)
    init_glyph_index(G_SPIDER, 's', 305)
    init_glyph_index(G_TENTACLE_HORROR, 'H', 306)
    init_glyph_index(G_TOAD, 't', 307)
    init_glyph_index(G_TOTEM, 128 + 10, 308)
    init_glyph_index(G_TROLL, 'T', 309)
    init_glyph_index(G_TURRET, 128 + 9, 310)
    init_glyph_index(G_UNDERWORM, 'U', 311)
    init_glyph_index(G_UNICORN, 218, 312)
    init_glyph_index(G_VAMPIRE, 'V', 313)
    init_glyph_index(G_WARDEN, 'Y', 314)
    init_glyph_index(G_WINGED_GUARDIAN, 223, 315)
    init_glyph_index(G_WISP, 'w', 316)
    init_glyph_index(G_WRAITH, 'W', 317)
    init_glyph_index(G_ZOMBIE, 'Z', 318)
    init_glyph_index(G_GOOD_MAGIC, 128 + 13, 319)
    init_glyph_index(G_GOOD_ITEM, 128 + 13, 320)
    init_glyph_index(G_BAD_MAGIC, 128 + 12, 321)
    init_glyph_index(G_BAD_ITEM, 128 + 12, 322)
    init_glyph_index(G_AMULET, 128 + 5, 323)
    init_glyph_index(G_AMULET_ITEM, 128 + 5, 324)
    init_glyph_index(G_FLOOR, '.', 325)
    init_glyph_index(G_FLOOR_ALT, '.', 326)
    init_glyph_index(G_LIQUID, '~', 327)
    init_glyph_index(G_CHASM, 128 + 1, 328)
    init_glyph_index(G_HOLE, 128 + 1, 329)
    init_glyph_index(G_FIRE, 128 + 3, 330)
    init_glyph_index(G_TORCH, '#', 331)
    init_glyph_index(G_FALLEN_TORCH, '#', 332)
    init_glyph_index(G_ASHES, '\'', 333)
    init_glyph_index(G_RUBBLE, ',', 334)
    init_glyph_index(G_BONES, ',', 335)
    init_glyph_index(G_CARPET, '.', 336)
    init_glyph_index(G_BOG, ',', 337)
    init_glyph_index(G_BEDROLL, '=', 338)
    init_glyph_index(G_GRASS, '"', 339)
    init_glyph_index(G_FOLIAGE, 128 + 4, 340)
    init_glyph_index(G_BLOODWORT_STALK, 128 + 4, 341)
    init_glyph_index(G_BLOODWORT_POD, '*', 342)
    init_glyph_index(G_SACRED_GLYPH, 128 + 1, 343)
    init_glyph_index(G_GLOWING_GLYPH, 128 + 1, 344)
    init_glyph_index(G_MAGIC_GLYPH, 128 + 1, 345)
    init_glyph_index(G_STATUE, 223, 346)
    init_glyph_index(G_CRACKED_STATUE, 223, 347)
    init_glyph_index(G_WALL, '#', 348)
    init_glyph_index(G_WALL_TOP, '#', 349)
    init_glyph_index(G_GRANITE, '#', 350)
    init_glyph_index(G_CRYSTAL_WALL, '#', 351)
    init_glyph_index(G_CRYSTAL, '#', 352)
    init_glyph_index(G_ELECTRIC_CRYSTAL, 164, 353)
    init_glyph_index(G_OPEN_DOOR, '\'', 354)
    init_glyph_index(G_CLOSED_DOOR, '+', 355)
    init_glyph_index(G_OPEN_IRON_DOOR, '\'', 356)
    init_glyph_index(G_CLOSED_IRON_DOOR, '+', 357)
    init_glyph_index(G_PORTCULLIS, '#', 358)
    init_glyph_index(G_DOORWAY, 144 + 6, 359)
    init_glyph_index(G_UP_STAIRS, '<', 360)
    init_glyph_index(G_DOWN_STAIRS, '>', 361)
    init_glyph_index(G_BRIDGE, '=', 362)
    init_glyph_index(G_BARRICADE, '#', 363)
    init_glyph_index(G_KEY, '-', 364)
    init_glyph_index(G_DEWAR, '&', 365)
    init_glyph_index(G_CLOSED_CAGE, '#', 366)
    init_glyph_index(G_OPEN_CAGE, '|', 367)
    init_glyph_index(G_ALTAR, '|', 368)
    init_glyph_index(G_LEVER, '/', 369)
    init_glyph_index(G_LEVER_PULLED, '\\', 370)
    init_glyph_index(G_TRAP, 128 + 2, 371)
    init_glyph_index(G_PRESSURE_PLATE_INACTIVE, 128 + 2, 372)
    init_glyph_index(G_TRAP_GAS, 128 + 2, 373)
    init_glyph_index(G_TRAP_PARALYSIS, 128 + 2, 374)
    init_glyph_index(G_TRAP_CONFUSION, 128 + 2, 375)
    init_glyph_index(G_TRAP_FIRE, 128 + 2, 376)
    init_glyph_index(G_TRAP_FLOOD, 128 + 2, 377)
    init_glyph_index(G_TRAP_NET, 128 + 2, 378)
    init_glyph_index(G_PRESSURE_PLATE, 128 + 2, 379)
    init_glyph_index(G_TRAP_ALARM, 128 + 2, 380)
    init_glyph_index(G_VENT, '=', 381)
    init_glyph_index(G_WEB, ':', 382)
    init_glyph_index(G_NET, ':', 383)
    init_glyph_index(G_VINE, ':', 384)
    init_glyph_index(G_GEM, 128 + 9, 385)
    init_glyph_index(G_PEDESTAL, '|', 386)
    init_glyph_index(G_CLOSED_COFFIN, '|', 387)
    init_glyph_index(G_OPEN_COFFIN, '|', 388)
    init_glyph_index(G_CHAIN_LEFT, '-', 389)
    init_glyph_index(G_CHAIN_TOP, '|', 390)
    init_glyph_index(G_CHAIN_TOP_LEFT, '\\', 391)
    init_glyph_index(G_CHAIN_BOTTOM_LEFT, '/', 392)
    init_glyph_index(G_CHAIN_RIGHT, '-', 393)
    init_glyph_index(G_CHAIN_BOTTOM, '|', 394)
    init_glyph_index(G_CHAIN_BOTTOM_RIGHT, '\\', 395)
    init_glyph_index(G_CHAIN_TOP_RIGHT, '/', 396)
}

boolean init_font() {
    char font_path[PATH_MAX];
    (void) realpath("../custom.ttf", font_path);
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
            int minx,maxx,miny,maxy;
            TTF_GlyphMetrics(font,FONT_BOUND_CHAR,&minx,&maxx,&miny,&maxy,NULL);
            font_width = maxx - minx;
            font_height = maxy - miny;
            init_glyph_index_table();
            return true;
        }
    }
}

void to_buffer(uchar ch,
                 short xLoc, short yLoc,
                 short foreRed, short foreGreen, short foreBlue,
                 short backRed, short backGreen, short backBlue){
   displayBuffer[xLoc][yLoc] = (cellDisplayBuffer) {.character = ch,
                                                    .foreColorComponents = {foreRed,foreGreen,foreBlue},
                                                    .backColorComponents = {backRed,backGreen,backBlue},
                                                    .needsUpdate = true
   };
}

void redraw_value(int index){
    setting * s = & setting_list[index];
    int start = s->xLoc + SETTING_NAME_MAX_LEN;
    char buffer[SETTING_VALUE_MAX_LEN-2] = {0};
    switch(s->t){
        case boolean_:
            strcpy(buffer,s->new.b?"true ":"false");
            break;
        case int_:
            sprintf(buffer,"%d",s->new.i);
            break;
        case double_:
            sprintf(buffer,"%.1f",s->new.d);
            break;
        case section_:
        case button_:
            break;
    }
    if(s->t != section_) {
        to_buffer('<', start, s->yLoc, 0, 0, 0, 60, 60, 60);
        to_buffer('>', start + SETTING_VALUE_MAX_LEN - 1, s->yLoc, 0, 0, 0, 60, 60, 60);
        for (int i = 0; i < SETTING_VALUE_MAX_LEN - 2; i++) {
            to_buffer(' ', i + start + 1, s->yLoc, 0, 0, 0, 100, 100, 100);
        }
        int offset = (SETTING_VALUE_MAX_LEN - 2 - strlen(buffer)) / 2;
        for (int i = 0;buffer[i]; i++) {
            to_buffer(buffer[i], i + start + 1 + offset, s->yLoc, 0, 0, 0, 100, 100, 100);
        }


    }
}
void rebuild_settings_menu(int current_section){ //-1 means no section is open
    clearDisplayBuffer(displayBuffer);
    short xLoc=3,yLoc=1;
    int section_no = 0;
    for(int i=0;i<setting_len;i++){
        if(setting_list[i].t == section_){
            section_no += 1;
        }else if(setting_list[i].t != button_ &&  section_no != (current_section+1)){
            setting_list[i].yLoc = -1;
            continue;
        }
        if(setting_list[i].t != button_){
            setting_list[i].xLoc = xLoc;
            setting_list[i].yLoc = yLoc;
        }
        redraw_value(i);
        char * name = setting_list[i].name;
        char bg=0,fg=100;
        if(setting_list[i].t == section_ || setting_list[i].t == button_){
            bg = 100;
            fg = 0;
        }

        for(int j=0;name[j];j++){
            to_buffer(name[j]!='_'?name[j]:' ',
                      setting_list[i].xLoc + j,setting_list[i].yLoc,
                      fg,fg,fg,
                      bg,bg,bg
            );
        }
        if(setting_list[i].t != button_) {
            yLoc += 3;
            if (yLoc >= ROWS - 2) {
                xLoc += SETTING_NAME_MAX_LEN + SETTING_VALUE_MAX_LEN + 2;
                yLoc = 1;
            }
        }
    }
    refreshScreen();
    if(screen_changed) {
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_SetRenderTarget(renderer, screen_texture);
    }
}

void settings_menu() {
    restart_game = true;
    settings_changed = false;
    int current_section = 0;
    int hold = 0;
    int acc = 0;
    int16_t cursor_x,cursor_y;
    for(int i=0;i<setting_len;i++){
        setting * s = & setting_list[i];
        switch(s->t){
            case boolean_:
                s->new.b = * (boolean *) s->value;
                break;
            case int_:
                s->new.i = * (int *) s->value;
                break;
            case double_:
                s->new.d = * (double *) s->value;
                break;
            case section_:
            case button_:
                break;
        }
    }
    rebuild_settings_menu(current_section);
    while(true){
        SDL_Event event;
        while (SDL_PollEvent(&event)){
            if (event.type == SDL_FINGERDOWN){
                double raw_input_x,raw_input_y;
                raw_input_x = event.tfinger.x * display.w;
                raw_input_y = event.tfinger.y * display.h;
                cursor_x = min(COLS - 1, raw_input_x / cell_w);
                cursor_y = min(ROWS - 1, raw_input_y / cell_h);
                hold = SDL_GetTicks();

            } else if (event.type == SDL_FINGERUP){
                hold = 0;
            }
        }
        if(hold){
            acc +=1;
            for(int i=0;i<setting_len;i++){
                setting * s = &setting_list[i];
                if(abs(s->yLoc-cursor_y)<=1 && (s->xLoc <= cursor_x && cursor_x <= s->xLoc + SETTING_NAME_MAX_LEN + SETTING_VALUE_MAX_LEN  )){
                    boolean decrease = abs(cursor_x - s->xLoc - SETTING_NAME_MAX_LEN) <= 2;
                    boolean increase = cursor_x >= s->xLoc + SETTING_NAME_MAX_LEN + SETTING_VALUE_MAX_LEN - 2;
                    boolean menu_changed = false;
                    FILE * st;
                    switch(s->t){
                        case section_:
                            current_section = s->default_.s;
                            cursor_x = -1;
                            menu_changed = true;
                            break;
                        case button_:
                            switch (s->default_.id){
                                case DEFAULTS_BUTTON_ID:
                                    for(int i=0;i<setting_len;i++){
                                        setting * s = &setting_list[i];
                                        s->new = s->default_;
                                    }
                                    menu_changed = true;
                                    break;
                                case CANCEL_BUTTON_ID:
                                    return;
                                case OK_BUTTON_ID:
                                    st = fopen("../" SETTINGS_FILE,"w");
                                    for(int i=0;i<setting_len;i++){
                                        setting * s = &setting_list[i];
                                        switch(s->t){
                                            case boolean_:
                                                if(* (( boolean * ) s->value) != s->new.b){
                                                    restart_game = restart_game || s->need_restart;
                                                    *((boolean *) s->value) = s->new.b;
                                                    settings_changed = true;
                                                }
                                                if(s->new.b != s->default_.b) {
                                                    fprintf(st, "%s %s\n",s->name, s->new.b ? "1" : "0");
                                                }
                                                break;
                                            case int_:
                                                if(* (( int * ) s->value) != s->new.i){
                                                    restart_game = restart_game || s->need_restart;
                                                    *((int *) s->value) = s->new.i;
                                                    settings_changed = true;
                                                }
                                                if(s->new.i != s->default_.i) {
                                                    fprintf(st, "%s %d\n",s->name, s->new.i);
                                                }
                                                break;
                                            case double_:
                                                if(* (( double * ) s->value) != s->new.d){
                                                    restart_game = restart_game || s->need_restart;
                                                    *((double *) s->value) = s->new.d;
                                                    settings_changed = true;
                                                }
                                                if(s->new.d != s->default_.d) {
                                                    fprintf(st, "%s %f\n",s->name, s->new.d);
                                                }
                                                break;
                                            case section_:
                                            case button_:
                                                break;
                                        }
                                    }
                                    fclose(st);
                                    return;

                            }
                            break;
                        case boolean_:
                            if (increase || decrease){
                                s->new.b = !s->new.b;
                                menu_changed = true;
                            }
                            break;
                        case int_:
                            if(decrease){
                                s->new.i = max(s->min_.i,s->new.i-1);
                                menu_changed = true;
                            }else if(increase){
                                s->new.i = min(s->max_.i,s->new.i+1);
                                menu_changed = true;
                            }
                            break;
                        case double_:
                            s->new.d = floorf(s->new.d*10) / 10;
                            if(decrease){
                                s->new.d = max(s->min_.d,s->new.d-0.1);
                                menu_changed = true;
                            }else if(increase){
                                s->new.d = min(s->max_.d,s->new.d+0.1);
                                menu_changed = true;
                            }
                            break;
                    }
                    if(menu_changed){
                        rebuild_settings_menu(current_section);
                    }
                }
            }

        }else{
            acc = 0;
        }
        SDL_Delay(100/max(1,(acc - 10)*2));
    }
}

boolean process_events() {
    static int16_t cursor_x = 0;
    static int16_t cursor_y = 0;
    static uint32_t ctrl_time = 0;
    static uint32_t finger_down_time = 0;
    static uint32_t zoom_changed_time = 0;
    static uint32_t zoom_toggled_time = 0;
    static boolean virtual_keyboard = false;
    static boolean on_dpad = false;
    static bool_store prev_zoom_toggle = unset;
    static boolean in_left_panel = true;
    if(resumed){
        resumed = false;
        destroy_assets();
        create_assets();
        refreshScreen();
    }
    if(current_event.eventType!=EVENT_ERROR){
        return true;
    }

    current_event.shiftKey = false;
    current_event.controlKey = ctrl_pressed;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        double raw_input_x,raw_input_y;
        switch (event.type) {
            case SDL_FINGERDOWN:
                if(event.tfinger.fingerId !=0){
                    current_event.eventType=EVENT_ERROR;
                    if(event.tfinger.fingerId == 1){
                        zoom_toggled_time = SDL_GetTicks();
                    }else if(event.tfinger.fingerId == 2){
                        if(!ctrl_pressed){
                            current_event.controlKey = ctrl_pressed = true;
                            ctrl_time = SDL_GetTicks();
                        }
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
                if(!gameStat.titleMenuShown && SDL_PointInRect(&p, &dpad_area)){
                    on_dpad = true;
                    finger_down_time = SDL_GetTicks();
                    break;

                }
                if(gameStat.titleMenuShown && SDL_PointInRect(&p, &settings_icon_area)){
                    settings_menu();
                    rogue.nextGame = NG_QUIT;
                    break;
                }
                if(is_zoomed() && SDL_PointInRect(&p,&grid_box)){
                    raw_input_x = (raw_input_x-grid_box.x)/zoom_level + grid_box_zoomed.x;
                    raw_input_y = (raw_input_y-grid_box.y)/zoom_level + grid_box_zoomed.y;
                }
                if(!double_tap_lock || SDL_TICKS_PASSED(SDL_GetTicks(),finger_down_time+double_tap_interval)){
                    cursor_x = min(COLS - 1,raw_input_x / cell_w);
                    cursor_y = min(ROWS -1,raw_input_y / cell_h);
                }
                in_left_panel = false;
                if(smart_zoom && left_panel_smart_zoom && SDL_PointInRect(&p,&left_panel_box) && cursor_x > LEFT_EDGE_WIDTH){
                    if(cursor_y <= gameStat.sideBarLength){
                        in_left_panel = true;
                        if(prev_zoom_toggle == unset) {
                            prev_zoom_toggle = zoom_toggle ? set_true : set_false;
                        }
                    }
                }

                current_event.param1 = cursor_x;
                current_event.param2 = cursor_y;
                current_event.eventType = MOUSE_DOWN;
                finger_down_time = SDL_GetTicks();
                break;
            case SDL_FINGERUP:
                if(event.tfinger.fingerId !=0){
                    current_event.eventType=EVENT_ERROR;
                    if(!ctrl_pressed && event.tfinger.fingerId == 1 && !SDL_TICKS_PASSED(SDL_GetTicks(),zoom_toggled_time+ZOOM_TOGGLED_INTERVAL)){
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
                    if(finger_down_time == 0){
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
                            }else if(rogue.playbackMode){
                                current_event.param1 = ACKNOWLEDGE_KEY;
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
                if(current_event.param1 < LEFT_EDGE_WIDTH){
                    if(current_event.param2 < 2){
                        current_event.eventType = KEYSTROKE;
                    if(rogue.playbackMode){
                        current_event.param1 = ACKNOWLEDGE_KEY;
                    }else {
                        current_event.param1 = ENTER_KEY;
                    }

                    }else if(current_event.param2 > (ROWS - 3)){
                        current_event.eventType = KEYSTROKE;
                        current_event.param1 = ESCAPE_KEY;
                    }else{
                        virtual_keyboard = true;
                        start_text_input();
                    }
                }else{
                    current_event.eventType = MOUSE_UP;
                }
                if(!virtual_keyboard){
                    stop_text_input();
                }
                break;
            case SDL_FINGERMOTION: //For long press check
                if(!SDL_TICKS_PASSED(SDL_GetTicks(),zoom_changed_time+ZOOM_CHANGED_INTERVAL)){
                    current_event.eventType = EVENT_ERROR;
                    break;
                }
                if(finger_down_time != 0 && SDL_TICKS_PASSED(SDL_GetTicks(),finger_down_time + long_press_interval)){
                    if(on_dpad && allow_dpad_mode_change){
                        dpad_mode = !dpad_mode;
                        screen_changed = true;
                        pauseForMilliseconds(0);
                    }
                    finger_down_time = 0;
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
                break;
            case SDL_KEYDOWN:
                current_event.eventType = KEYSTROKE;
                SDL_KeyCode k = event.key.keysym.sym;
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
    boolean dialog_popup = !smart_zoom_allowed();
    if(in_left_panel || dialog_popup){
        if(prev_zoom_toggle == unset){
           prev_zoom_toggle = zoom_toggle?set_true:set_false;
        }
        zoom_toggle = false;
    }else if(prev_zoom_toggle != unset){
        zoom_toggle = prev_zoom_toggle == set_true?true:false;
        prev_zoom_toggle = unset;
    }
    return current_event.eventType != EVENT_ERROR;
}


void TouchScreenGameLoop() {
    restart_game = true;
    settings_changed = false;
    do {
        if(restart_game) {
            display = screen;
            if (force_portrait) {
                SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "Portrait PortraitUpsideDown",SDL_HINT_OVERRIDE);
                int tmp = display.w;
                display.w = display.h;
                display.h = tmp;
            }else{
                SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight",SDL_HINT_OVERRIDE);
            }
            char render_hint[2] = {filter_mode + '0', 0};
            SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, render_hint,SDL_HINT_OVERRIDE);
            if (custom_cell_width != 0) {
                cell_w = custom_cell_width;
            } else {
                cell_w = ((double) display.w) / COLS;
            }
            if (custom_cell_height != 0) {
                cell_h = custom_cell_height;
            } else {
                cell_h = ((double) display.h) / ROWS;
            }
            left_panel_box = (SDL_Rect) {.x = 0, .y = 0, .w = LEFT_PANEL_WIDTH * cell_w, .h = ROWS *
                                                                                              cell_h};
            log_panel_box = (SDL_Rect) {.x = LEFT_PANEL_WIDTH * cell_w, .y = 0, .w =
            (COLS - LEFT_PANEL_WIDTH) * cell_w, .h = TOP_LOG_HEIGIHT * cell_h};
            button_panel_box = (SDL_Rect) {.x = LEFT_PANEL_WIDTH * cell_w, .y =
            (ROWS - BOTTOM_BUTTONS_HEIGHT) * cell_h, .w = (COLS - LEFT_PANEL_WIDTH) * cell_w, .h =
            BOTTOM_BUTTONS_HEIGHT * cell_h};
            grid_box = grid_box_zoomed = (SDL_Rect) {.x = LEFT_PANEL_WIDTH * cell_w, .y =
            TOP_LOG_HEIGIHT * cell_h, .w = (COLS - LEFT_PANEL_WIDTH) * cell_w, .h=
            (ROWS - TOP_LOG_HEIGIHT - BOTTOM_BUTTONS_HEIGHT) * cell_h};
            create_assets();
            if (!init_font()) {
                critical_error("Font Error",
                               "Resolution/cell size is too small for minimum allowed font size");
            }
        }else if(settings_changed){
            create_assets();
        }
        settings_changed = restart_game = false;
        rogue.nextGame = NG_NOTHING;
        rogue.nextGamePath[0] = '\0';
        rogue.nextGameSeed = 0;
        rogueMain();
        destroy_assets();
    }while(settings_changed || restart_game);

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

        if(!is_zoomed()){
            SDL_RenderCopy(renderer, screen_texture, NULL, NULL);

        }else{
            double width = (COLS - LEFT_PANEL_WIDTH) * cell_w / zoom_level;
            double height = (ROWS - TOP_LOG_HEIGIHT - BOTTOM_BUTTONS_HEIGHT)*cell_h/zoom_level;
            int x,y;
            if(zoom_mode == 2 && rogue.cursorLoc[0] >= 0 && rogue.cursorLoc[1] >= 0 &&
                !rogue.automationActive && !rogue.autoPlayingLevel && rogue.disturbed){
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
        if(gameStat.titleMenuShown){
            SDL_RenderCopy(renderer,settings_image,NULL,&settings_icon_area);
        } else if(dpad_enabled){
            SDL_RenderCopy(renderer, dpad_mode?dpad_image_move:dpad_image_select, NULL, &dpad_area);
        }

        SDL_RenderPresent(renderer);
        SDL_SetRenderTarget(renderer, screen_texture);
    }
    uint32_t epoch = SDL_GetTicks() - init_time;
    if(epoch < milliseconds){
        SDL_Delay(milliseconds - epoch);
    }
    return process_events();
}

void TouchScreenNextKeyOrMouseEvent(rogueEvent *returnEvent, boolean textInput, boolean colorsDance) {
    while(!process_events()){
        boolean screen_changed = false;
        if (dynamic_colors && colorsDance) {
            shuffleTerrainColors(3, true);
            screen_changed = true;
        }
        boolean refresh = false;
        if (graphicsEnabled && tiles_animation){
            static uint32_t prev_time = 0;
            uint32_t current_time = SDL_GetTicks();
            if(current_time >= (prev_time+TILES_FLIP_TIME)){
                tiles_flipped = !tiles_flipped;
                prev_time = current_time;
                refresh = true;
            }
            //Do not refresh screen during zoom change
            static double prev_zoom = 0.0;
            if(refresh && prev_zoom == zoom_level){
                for(int i=0; i<COLS; i++ ) {
                    for(int j=0; j<ROWS; j++ ) {
                        displayBuffer[i][j].needsUpdate = font_cache[displayBuffer[i][j].character].animated;
                    }
                }
                screen_changed = true;
            }
            prev_zoom = zoom_level;
        }else{
            tiles_flipped = false;
        }
        if(screen_changed){
            commitDraws();
        }
        TouchScreenPauseForMilliseconds(FRAME_INTERVAL);

    }
    *returnEvent = current_event;
    current_event.eventType = EVENT_ERROR; //unset the event
}


void TouchScreenPlotChar(enum displayGlyph ch,
                         short xLoc, short yLoc,
                         short foreRed, short foreGreen, short foreBlue,
                         short backRed, short backGreen, short backBlue) {

    SDL_FRect rect;
    rect.x = xLoc * cell_w;
    rect.y = yLoc * cell_h;
    rect.w = cell_w;
    rect.h = cell_h;
    SDL_SetRenderDrawColor(renderer, convert_color(backRed), convert_color(backGreen),convert_color(backBlue), COLOR_MAX);
    SDL_RenderFillRectF(renderer, &rect);
    draw_glyph(ch, rect, convert_color(foreRed), convert_color(foreGreen), convert_color(foreBlue));
    screen_changed = true;
}

void TouchScreenRemap(const char *input_name, const char *output_name) {
}

boolean TouchScreenModifierHeld(int modifier) {
    //Shift key check is unnecessary since function is not called with modifier == 0 in anywhere
    return modifier == 1 && ctrl_pressed;
}


static boolean TouchScreenSetGraphicsEnabled(boolean state) {
    graphicsEnabled = state;
    refreshScreen();
    return state;
}

void TouchScreenTextInputStart(){
    if(!virtual_keyboard_active){ //Open keyboard if it is not opened by demand previously
        requires_text_input = true;
        start_text_input();
    }
}

void TouchScreenTextInputStop(){
    if(requires_text_input){
        requires_text_input = false;
        stop_text_input();
    }
}

struct brogueConsole TouchScreenConsole = {
        .gameLoop = TouchScreenGameLoop,
        .pauseForMilliseconds = TouchScreenPauseForMilliseconds,
        .nextKeyOrMouseEvent = TouchScreenNextKeyOrMouseEvent,
        .plotChar = TouchScreenPlotChar,
        .remap = TouchScreenRemap,
        .modifierHeld = TouchScreenModifierHeld,
        .setGraphicsEnabled = TouchScreenSetGraphicsEnabled,
        .textInputStart = TouchScreenTextInputStart,
        .textInputStop = TouchScreenTextInputStop,
};

boolean git_version_check(JNIEnv * env,jobject activity,jclass cls){
    if(ask_for_update_check) {
        const SDL_MessageBoxButtonData buttons[] = {
                {0,                                       0, "Later"},
                {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Check Now"},
        };

        const SDL_MessageBoxData messageboxdata = {
                SDL_MESSAGEBOX_INFORMATION,
                NULL,
                "Check New Version",
                "Do you want to check new version now?",
                SDL_arraysize(buttons),
                buttons,
                NULL,
        };
        int buttonid;
        SDL_ShowMessageBox(&messageboxdata, &buttonid);
        if (buttonid == 0) {
            return true;
        }
    }
    jmethodID method_id = (*env)->GetMethodID(env,cls, "gitVersionCheck", "()Ljava/lang/String;");
    jstring ver_ = (*env)->CallObjectMethod(env,activity, method_id);
    const char * ver = (*env)->GetStringUTFChars(env,ver_,NULL);
    char version_message[500];
    char error_title[] = "Cannot Get New Version Info";
    boolean return_value = false;
    switch(ver[0]){
        case ' ': //No Error
        if(strlen(ver)>1){
            sprintf(version_message,"Latest version %s released.Opening download page now",ver);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"New Version Found",version_message,NULL);
            jmethodID method_id = (*env)->GetMethodID(env,cls, "openDownloadLink", "()V");
            (*env)->CallVoidMethod(env,activity, method_id);
        }else if (ask_for_update_check){
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"No new Version","Already using latest version",NULL);
        }
        return_value = true;
        break;
        case '1': //Timeout Error
            if(ask_for_update_check)
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,error_title,"Connection timed out",NULL);
            break;
        case '2': //JSON Error

            if(ask_for_update_check)
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,error_title,"Cannot parse json",NULL);
            break;
        case '3': //Connection Error
            if(ask_for_update_check)
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,error_title,"Cannot download version info",NULL);
            break;

    }
    (*env)->ReleaseStringUTFChars(env,ver_,ver);
    return return_value;
}

void open_manual(JNIEnv * env,jobject activity,jclass cls){
    const SDL_MessageBoxButtonData buttons[] = {
            {0,                                       0, "No"},
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes"},
    };

    const SDL_MessageBoxData messageboxdata = {
              SDL_MESSAGEBOX_INFORMATION,
              NULL,
              "Manual",
              "Manual contains information about control, settings etc. Do you want to read it now?",
              SDL_arraysize(buttons),
            buttons,
              NULL,
    };
    int buttonid;
    SDL_ShowMessageBox(&messageboxdata, &buttonid);
    if (buttonid == 0) {
        return;
    }
    jmethodID method_id = (*env)->GetMethodID(env,cls, "openManual", "()V");
    (*env)->CallVoidMethod(env,activity, method_id);
}

void config_folder(JNIEnv * env,jobject activity,jclass cls,boolean first_run){
    jmethodID method_id = (*env)->GetMethodID(env,cls, "configFolder", "()Ljava/lang/String;");
    jstring folder_ = (*env)->CallObjectMethod(env,activity, method_id);
    if(folder_!= NULL){
        const char * folder = (*env)->GetStringUTFChars(env,folder_,NULL);
        chdir(folder);
        (*env)->ReleaseStringUTFChars(env,folder_,folder);
    }else{
        if(first_run) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Config Folder Error",
                                     "Cannot create or access sdcard/Brogue."
                                     "Will save to data folder of the app", NULL);
        }
        chdir(SDL_AndroidGetExternalStoragePath());
    }
}

int brogue_main(void *data){
    currentConsole = TouchScreenConsole;
    rogue.nextGame = NG_NOTHING;
    rogue.nextGamePath[0] = '\0';
    rogue.nextGameSeed = 0;
    currentConsole.gameLoop();
    return 0;
}

boolean serverMode = false;

int main() {
    chdir(SDL_AndroidGetInternalStoragePath());
    FILE * fc;
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = (*env)->GetObjectClass(env,activity);
    jmethodID method_id;
    boolean first_run = false;
    if(access("first_run",F_OK) == -1){
        method_id = (*env)->GetMethodID(env,cls, "needsWritePermission", "()Z");
        if((*env)->CallBooleanMethod(env,activity,method_id)){
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"Write Permission",
                                     "To use sdcard/Brogue as save folder you need to grant app write permission in "
                                     "Android 6.0+. Otherwise it will save the app will use the folder under Android/data",NULL);
            method_id = (*env)->GetMethodID(env,cls, "grantPermission", "()V");
            (*env)->CallVoidMethod(env,activity, method_id);
            SDL_Delay(1000);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"Continue?","",NULL);
        }
        open_manual(env,activity,cls);
        fc = fopen("first_run","w");
        fclose(fc);
        first_run = true;
    }

    time_t update_check_time = 0;
    if(access("last_update_check",F_OK) == -1){
        fc = fopen("last_update_check","w");
    }else{
        fc = fopen("last_update_check","r");
        fscanf(fc,"%ld",&update_check_time);
        fclose(fc);
        fc = fopen("last_update_check","w");
    }
    config_folder(env,activity,cls,first_run);
    set_conf("",""); //set default values of config
    load_conf();
    graphicsEnabled = tiles_by_default;
    if(check_update){
        time_t new_time;
        time(&new_time);
        if(new_time > update_check_time) {
            if ((new_time-update_check_time) > DAY_TO_TIMESTAMP * check_update_interval && git_version_check(env,activity,cls)) {
                update_check_time = new_time;
            }
        }
    }
    fprintf(fc, "%ld", update_check_time);
    fclose(fc);
    (*env)->DeleteLocalRef(env,activity);
    (*env)->DeleteLocalRef(env,cls);
    if(chdir(BROGUE_VERSION_STRING) == -1){
        if(errno != ENOENT){
            critical_error("Save Folder Error","Cannot create/enter the save folder");
        }
        if(mkdir(BROGUE_VERSION_STRING,0770) || chdir(BROGUE_VERSION_STRING)){
            critical_error("Save Folder Error","Cannot create/enter the save folder");
        }
    }
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        critical_error("SDL Error","Unable to initialize SDL: %s", SDL_GetError());
    }
    if (TTF_Init() != 0) {
        critical_error("SDL_ttf Error","Unable to initialize SDL_ttf: %s", SDL_GetError());
    }
    if (SDL_GetDisplayBounds(0, &screen) != 0) {
        critical_error("SDL Error","SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
    }
    SDL_SetEventFilter(suspend_resume_filter, NULL);
    SDL_Thread *thread = SDL_CreateThreadWithStackSize(brogue_main, "Brogue", 8 * 1024 * 1024,
                                                       NULL);
    if (thread != NULL) {
        int result;
        SDL_WaitThread(thread, &result);
    } else {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Thread Error",
                                 "Cannot create a thread with sufficient stack size. "
                                 "The game will start nonetheless but be aware that some seeds may cause the game "
                                 "to crash in this mode.", NULL);
        brogue_main(NULL);

    }
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    free(setting_list);
    exit(0); //return causes problems
}
