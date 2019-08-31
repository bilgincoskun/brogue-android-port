#include <unistd.h>
#include <limits.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "platform.h"

#define COLOR_MAX UCHAR_MAX
#define MAX(x,y)  ((x)>(y)?(x):(y))
#define MIN(x,y)  ((y)>(x)?(x):(y))

typedef struct {
    uint8_t fr, fg, fb, br, bg, bb;
    uint16_t c;
} cell;

typedef struct {
    SDL_Texture *c;
    int width, height, offset_x, offset_y;
} letter_cache;

struct brogueConsole currentConsole;
extern playerCharacter rogue;

SDL_Window *window;
SDL_Renderer *renderer;
SDL_DisplayMode display;
TTF_Font *font;
letter_cache font_cache[UCHAR_MAX];
cell grid[COLS][ROWS];
boolean screen_changed = false;
boolean ctrl_pressed = false;
int cell_w, cell_h;

uint8_t convert_color(short c) {
    c = c * COLOR_MAX / 100;
    return MAX(0,MIN(c,COLOR_MAX));
}

void draw_letter(uint16_t c, SDL_Rect rect, uint8_t r, uint8_t g, uint8_t b) {
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
    letter_cache *lc = &font_cache[key];
    if (!lc->c) {
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

void draw_cell(int x, int y) {
    cell c = grid[x][y];
    SDL_Rect rect;
    rect.x = x * cell_w;
    rect.y = y * cell_h;
    rect.w = cell_w;
    rect.h = cell_h;
    if(c.br != 0 || c.bg != 0 || c.bb != 0) { //Since background is black do not redraw
        SDL_SetRenderDrawColor(renderer, c.br, c.bg, c.bb, COLOR_MAX);
        SDL_RenderFillRect(renderer, &rect);
    }
    draw_letter(c.c, rect, c.fr, c.fg, c.fb);
}

TTF_Font *init_font_size(char *font_path, int size) {
    TTF_Font *current_font = TTF_OpenFont(font_path, size);
    if (TTF_FontLineSkip(current_font) <= (cell_h - 2)){
        int advance;
        if(!TTF_GlyphMetrics(current_font,'a',NULL,NULL,NULL,NULL,&advance) && advance <= (cell_w -2)){
            return current_font;
        }
    }
    TTF_CloseFont(current_font);
    return NULL;
}

int init_font() {
    char font_path[PATH_MAX];
    realpath("custom.ttf", font_path);
    if (access(font_path, F_OK) == -1) {
        strcpy(font_path, "default.ttf");
    }
    int size = 5;
    font = init_font_size(font_path, size);
    if (!font) {
        return 1;
    }
    while (true) {
        size++;
        TTF_Font *new_size = init_font_size(font_path, size);
        if (new_size) {
            TTF_CloseFont(font);
            font = new_size;
        } else {
            return 0;
        }
    }
}

void TouchScreenGameLoop() {
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
    if (SDL_CreateWindowAndRenderer(display.w, display.h, SDL_WINDOW_FULLSCREEN, &window,
                                    &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s",
                     SDL_GetError());
        return;
    }
    memset(grid, 0, ROWS * COLS * sizeof(cell));
    memset(font_cache, 0, UCHAR_MAX * sizeof(letter_cache));
    cell_w = display.w / COLS;
    cell_h = display.h / ROWS;
    if (init_font() != 0) {
        SDL_Log("Cannot fit font into cells");
        return;
    }
    rogueMain();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
}
boolean TouchScreenPauseForMilliseconds(short milliseconds){
    uint32_t init_time = SDL_GetTicks();
    //Since dirty rectangles are problematic in Android we have to redraw all screen
    if(screen_changed) {
        screen_changed = false;
        for (int i = 0; i < COLS; i++) {
            for (int j = 0; j < ROWS; j++) {
                draw_cell(i, j);
            }
        }
        SDL_RenderPresent(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0,0 , COLOR_MAX);
        SDL_RenderClear(renderer); //Optimize black rectangles by setting background as black
    }
    uint32_t epoch = SDL_GetTicks() - init_time;
    if(epoch < milliseconds){
        SDL_Delay(milliseconds - epoch);
    }
    return SDL_HasEvents(SDL_FINGERDOWN,SDL_FINGERUP) || SDL_HasEvent(SDL_KEYUP);
}
void
TouchScreenNextKeyOrMouseEvent(rogueEvent *returnEvent, boolean textInput, boolean colorsDance) {
    static int cursor_x = 0;
    static int cursor_y = 0;
    int new_x,new_y;
    static int ctrl_time = 0;
    static int long_press_time = 0;
    static boolean long_press_check = false;
    static virtual_keyboard = false;
    returnEvent->shiftKey = false;
    returnEvent->controlKey = ctrl_pressed;
    while(returnEvent->eventType==EVENT_ERROR) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case (SDL_FINGERDOWN):
                    cursor_x = returnEvent->param1 = MIN(COLS - 1,event.tfinger.x * display.w / cell_w);
                    cursor_y = returnEvent->param2 = MIN(ROWS -1,event.tfinger.y * display.h / cell_h);
                    returnEvent->eventType = event.type = MOUSE_DOWN;
                    long_press_time = SDL_GetTicks();
                    long_press_check = true;
                    break;
                case (SDL_FINGERUP):
                    virtual_keyboard = false;
                    returnEvent->param1 = cursor_x;
                    returnEvent->param2 = cursor_y;
                    if(returnEvent->param1 < 2){
                        if(returnEvent->param2 < 2){
                            returnEvent->eventType = KEYSTROKE;
                            returnEvent->param1 = ENTER_KEY;

                        }else if(returnEvent->param2 > (ROWS - 3)){
                            returnEvent->eventType = KEYSTROKE;
                            returnEvent->param1 = ESCAPE_KEY;

                        }else{
                            virtual_keyboard = true;
                            SDL_StartTextInput();
                        }
                    }else{
                        returnEvent->eventType = event.type = MOUSE_UP;
                    }
                    if(!virtual_keyboard){
                        SDL_StopTextInput();
                    }
                    break;
                case (SDL_FINGERMOTION):
                    if(long_press_check && SDL_TICKS_PASSED(SDL_GetTicks(),long_press_time + 300)){
                        new_x =  MIN(COLS - 1,event.tfinger.x * display.w / cell_w);
                        new_y =  MIN(ROWS -1,event.tfinger.y * display.h / cell_h);
                        if(new_x == cursor_x && new_y == cursor_y){
                            returnEvent->param1 = cursor_x;
                            returnEvent->param2 = cursor_y;
                            returnEvent->eventType = event.type = MOUSE_UP;
                        }
                        long_press_check = false;
                    }

                    break;



                case (SDL_MULTIGESTURE):
                    if(event.mgesture.numFingers==3){
                        if(!ctrl_pressed){
                            returnEvent->controlKey = ctrl_pressed = true;
                            ctrl_time = SDL_GetTicks();
                        }
                    }
                    break;
                case (SDL_KEYUP):
                    returnEvent->eventType = KEYSTROKE;
                    SDL_Scancode k = event.key.keysym.sym;
                    switch(k){
                        case SDLK_AC_BACK:
                            returnEvent->param1 = ESCAPE_KEY;
                            break;
                        case  SDLK_BACKSPACE:
                        case  SDLK_DELETE:
                            returnEvent->param1 = DELETE_KEY;
                            break;
                        default:
                            returnEvent->param1 = k;
                            break;
                    }
                    break;
            }
        }
        if(SDL_TICKS_PASSED(SDL_GetTicks(),ctrl_time + 1000)){
            //Wait for 1 second to disable CTRL after pressing and another input
            returnEvent->controlKey = ctrl_pressed = false;
        }
        if (colorsDance) {
            shuffleTerrainColors(3, true);
            commitDraws();
        }
        TouchScreenPauseForMilliseconds(100);
    }
}

void TouchScreenPlotChar(uchar ch,
                         short xLoc, short yLoc,
                         short foreRed, short foreGreen, short foreBlue,
                         short backRed, short backGreen, short backBlue) {

    cell currentCell = {
            .fr = convert_color(foreRed),
            .fg = convert_color(foreGreen),
            .fb = convert_color(foreBlue),
            .br = convert_color(backRed),
            .bg = convert_color(backGreen),
            .bb = convert_color(backBlue),
            .c = ch,
    };
    screen_changed = true;
    grid[xLoc][yLoc] = currentCell;
}

void TouchScreenRemap(const char *input_name, const char *output_name) {
}

boolean TouchScreenModifierHeld(int modifier) {
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
