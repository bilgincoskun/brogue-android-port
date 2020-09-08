#ifndef _display_h_
#define _display_h_

#include <SDL.h>
#include "Rogue.h"

#define COLOR_MAX UCHAR_MAX
#define LEFT_PANEL_WIDTH 20
#define LEFT_EDGE_WIDTH 2
#define TOP_LOG_HEIGIHT 3
#define BOTTOM_BUTTONS_HEIGHT 2

extern SDL_Renderer *renderer;
extern boolean graphicsEnabled;
extern boolean tiles_flipped; //ASCII, tile frame 1, tile frame 2
extern double cell_w, cell_h;
extern boolean screen_changed;
extern double zoom_level;
extern boolean zoom_toggle;
extern boolean game_started;
extern SDL_Texture * screen_texture;
extern SDL_Rect left_panel_box;
extern SDL_Rect log_panel_box;
extern SDL_Rect button_panel_box;
extern SDL_Rect grid_box;
extern SDL_Rect grid_box_zoomed;
extern SDL_Texture * dpad_image_select;
extern SDL_Texture * dpad_image_move;
extern SDL_Rect dpad_area;
extern SDL_Texture * settings_image;
extern SDL_Rect settings_icon_area;


boolean init_font();
void destroy_font();
void draw_glyph(enum displayGlyph c, struct SDL_FRect rect, __uint8_t r, __uint8_t g, __uint8_t b);
boolean is_glyph_animated(enum displayGlyph c);
void draw_screen();
boolean smart_zoom_allowed();
boolean is_zoomed();

#endif
