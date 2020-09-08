#include <SDL_ttf.h>
#include "display.h"
#include "config.h"
#include <unistd.h>
#include "IncludeGlobals.h"

boolean graphicsEnabled = true;
SDL_Renderer *renderer;
double cell_w, cell_h;
boolean screen_changed = false;
double zoom_level = 1.0;
boolean zoom_toggle = false;
boolean game_started = false;
SDL_Texture * screen_texture;
SDL_Texture * dpad_image_select;
SDL_Texture * dpad_image_move;
SDL_Rect dpad_area;
SDL_Texture * settings_image;
SDL_Rect settings_icon_area;
SDL_Texture * screen_texture;
SDL_Rect left_panel_box;
SDL_Rect log_panel_box;
SDL_Rect button_panel_box;
SDL_Rect grid_box;
SDL_Rect grid_box_zoomed;

extern playerCharacter rogue;
extern creature player;

#define init_glyph_index(glyph,ascii,tile) \
    glyph_index_table[glyph-MIN_TILE][0] = ascii; \
    glyph_index_table[glyph-MIN_TILE][1] = tile; \
    glyph_index_table[glyph-MIN_TILE][2] = tile + TILES_LEN;
#define FONT_BOUND_CHAR 139
#define TILES_LEN 256
#define MAX_GLYPH_NO (TILES_LEN * 3)
#define MIN_TILE G_UP_ARROW
typedef struct {
    double width, height, offset_x, offset_y;
    SDL_Texture *c;
    boolean animated;
    boolean full_tile;
} glyph_cache;
static TTF_Font *font;
static glyph_cache font_cache[MAX_GLYPH_NO];
static uint16_t glyph_index_table[TILES_LEN][3];
static int font_width;
static int font_height;


boolean tiles_flipped = false;

void draw_glyph(enum displayGlyph c, struct SDL_FRect rect, uint8_t r, uint8_t g, uint8_t b) {
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
    glyph_cache * lc = &font_cache[key];
    if (lc->c == NULL) {
        struct SDL_Color fc = {COLOR_MAX, COLOR_MAX, COLOR_MAX};
        struct SDL_Surface *text;
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
        struct SDL_Rect src = {.x = 0,.y=0,.w=font_width,.h=font_height};
        SDL_RenderCopyF(renderer, lc->c, &src, &rect);
    }else{
        struct SDL_Rect font_rect;
        font_rect.x = rect.x + lc->offset_x;
        font_rect.y = rect.y + lc->offset_y;
        font_rect.w =  lc->width;
        font_rect.h = lc->height;
        SDL_RenderCopy(renderer, lc->c, NULL, &font_rect);
    }

}

struct _TTF_Font *init_font_size(char *font_path, int size) {
    struct _TTF_Font *current_font = TTF_OpenFont(font_path, size);
    if (TTF_FontLineSkip(current_font) <= (cell_h - 2)){
        int advance;
        if(TTF_GlyphMetrics(current_font,'a',NULL,NULL,NULL,NULL,&advance) == 0 && advance <= (cell_w -2)){
            return current_font;
        }
    }
    TTF_CloseFont(current_font);
    return NULL;
}

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
    memset(font_cache, 0, MAX_GLYPH_NO * sizeof(glyph_cache));
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
        struct _TTF_Font *new_size = init_font_size(font_path, size);
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

void destroy_font(){
    TTF_CloseFont(font);
}


boolean is_glyph_animated(enum displayGlyph c){
    return font_cache[c].animated;
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

void draw_screen(){
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

}
