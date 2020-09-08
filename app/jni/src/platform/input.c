#include "input.h"
#include <SDL.h>
#include "display.h"
#include "config.h"
#include "IncludeGlobals.h"

#define ZOOM_CHANGED_INTERVAL 300
#define ZOOM_TOGGLED_INTERVAL 100

typedef enum {
    set_false,
    set_true,
    unset,
} bool_store;

rogueEvent current_event;
boolean ctrl_pressed = false;
boolean requires_text_input = false;
boolean virtual_keyboard_active = false;

void start_text_input() {
  if (keyboard_visibility) {
    virtual_keyboard_active = true;
    SDL_StartTextInput();
  }
}

void stop_text_input() {
  if (keyboard_visibility != 2) {
    virtual_keyboard_active = false;
    SDL_StopTextInput();
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

  if (current_event.eventType != EVENT_ERROR) {
    return true;
  }

  current_event.shiftKey = false;
  current_event.controlKey = ctrl_pressed;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    double raw_input_x, raw_input_y;
    switch (event.type) {
    case SDL_FINGERDOWN:
      if (event.tfinger.fingerId != 0) {
        current_event.eventType = EVENT_ERROR;
        if (event.tfinger.fingerId == 1) {
          zoom_toggled_time = SDL_GetTicks();
        } else if (event.tfinger.fingerId == 2) {
          if (!ctrl_pressed) {
            current_event.controlKey = ctrl_pressed = true;
            ctrl_time = SDL_GetTicks();
          }
        }
        break;
      }
      if (!SDL_TICKS_PASSED(SDL_GetTicks(),
                            zoom_changed_time + ZOOM_CHANGED_INTERVAL)) {
        break;
      }
      on_dpad = false;
      raw_input_x = event.tfinger.x * display.w;
      raw_input_y = event.tfinger.y * display.h;
      SDL_Point p = {.x = raw_input_x, .y = raw_input_y};
      if (!gameStat.titleMenuShown && SDL_PointInRect(&p, &dpad_area)) {
        on_dpad = true;
        finger_down_time = SDL_GetTicks();
        break;
      }
      if (gameStat.titleMenuShown && SDL_PointInRect(&p, &settings_icon_area)) {
        settings_menu();
        rogue.nextGame = NG_QUIT;
        break;
      }
      if (is_zoomed() && SDL_PointInRect(&p, &grid_box)) {
        raw_input_x =
            (raw_input_x - grid_box.x) / zoom_level + grid_box_zoomed.x;
        raw_input_y =
            (raw_input_y - grid_box.y) / zoom_level + grid_box_zoomed.y;
      }
      if (!double_tap_lock ||
          SDL_TICKS_PASSED(SDL_GetTicks(),
                           finger_down_time + double_tap_interval)) {
        cursor_x = min(COLS - 1, raw_input_x / cell_w);
        cursor_y = min(ROWS - 1, raw_input_y / cell_h);
      }
      in_left_panel = false;
      if (smart_zoom && left_panel_smart_zoom &&
          SDL_PointInRect(&p, &left_panel_box) && cursor_x > LEFT_EDGE_WIDTH) {
        if (cursor_y <= gameStat.sideBarLength) {
          in_left_panel = true;
          if (prev_zoom_toggle == unset) {
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
      if (event.tfinger.fingerId != 0) {
        current_event.eventType = EVENT_ERROR;
        if (!ctrl_pressed && event.tfinger.fingerId == 1 &&
            !SDL_TICKS_PASSED(SDL_GetTicks(),
                              zoom_toggled_time + ZOOM_TOGGLED_INTERVAL)) {
          zoom_toggle = !zoom_toggle;
          screen_changed = true;
        }
        break;
      }
      if (!SDL_TICKS_PASSED(SDL_GetTicks(),
                            zoom_changed_time + ZOOM_CHANGED_INTERVAL)) {
        break;
      }
      raw_input_x = event.tfinger.x * display.w;
      raw_input_y = event.tfinger.y * display.h;
      if (dpad_enabled && on_dpad) {
        on_dpad = false;
        if (finger_down_time == 0) {
          break;
        }
        SDL_Point p = {.x = raw_input_x, .y = raw_input_y};
        if (SDL_PointInRect(&p, &dpad_area)) {
          int diff_x = 0, diff_y = 0;
          SDL_Rect min_x = {.x = dpad_area.x,
                            .y = dpad_area.y,
                            .w = dpad_area.w / 3,
                            .h = dpad_area.h};
          if (SDL_PointInRect(&p, &min_x)) {
            diff_x = -1;
          }
          SDL_Rect max_x = {.x = dpad_area.x + 2 * dpad_area.w / 3,
                            .y = dpad_area.y,
                            .w = dpad_area.w / 3,
                            .h = dpad_area.h};
          if (SDL_PointInRect(&p, &max_x)) {
            diff_x = 1;
          }
          SDL_Rect min_y = {.x = dpad_area.x,
                            .y = dpad_area.y,
                            .w = dpad_area.w,
                            .h = dpad_area.h / 3};
          if (SDL_PointInRect(&p, &min_y)) {
            diff_y = -1;
          }
          SDL_Rect max_y = {.x = dpad_area.x,
                            .y = dpad_area.y + 2 * dpad_area.h / 3,
                            .w = dpad_area.w,
                            .h = dpad_area.h / 3};
          if (SDL_PointInRect(&p, &max_y)) {
            diff_y = 1;
          }
          if (dpad_mode) {
            diff_y *= -1;
            current_event.eventType = KEYSTROKE;
            if (diff_x < 0) {
              if (diff_y < 0) {
                current_event.param1 = DOWNLEFT_KEY;
              } else if (diff_y > 0) {
                current_event.param1 = UPLEFT_KEY;
              } else {
                current_event.param1 = LEFT_KEY;
              }
            } else if (diff_x > 0) {
              if (diff_y < 0) {
                current_event.param1 = DOWNRIGHT_KEY;
              } else if (diff_y > 0) {
                current_event.param1 = UPRIGHT_KEY;
              } else {
                current_event.param1 = RIGHT_KEY;
              }
            } else if (diff_y < 0) {
              current_event.param1 = DOWN_KEY;
            } else if (diff_y > 0) {
              current_event.param1 = UP_KEY;
            } else if (rogue.playbackMode) {
              current_event.param1 = ACKNOWLEDGE_KEY;
            } else {
              current_event.param1 = RETURN_KEY;
            }
          } else {
            cursor_x =
                max(LEFT_PANEL_WIDTH + 1, min(COLS - 1, cursor_x + diff_x));
            cursor_y =
                max(TOP_LOG_HEIGIHT,
                    min(ROWS - (BOTTOM_BUTTONS_HEIGHT + 1), cursor_y + diff_y));
            current_event.param1 = cursor_x;
            current_event.param2 = cursor_y;
            current_event.eventType = MOUSE_ENTERED_CELL;
            if (!diff_x && !diff_y) {
              current_event.eventType = MOUSE_UP;
            }
          }
          break;
        }
      }
      virtual_keyboard = false;
      current_event.param1 = cursor_x;
      current_event.param2 = cursor_y;
      if (current_event.param1 < LEFT_EDGE_WIDTH) {
        if (current_event.param2 < 2) {
          current_event.eventType = KEYSTROKE;
          if (rogue.playbackMode) {
            current_event.param1 = ACKNOWLEDGE_KEY;
          } else {
            current_event.param1 = ENTER_KEY;
          }

        } else if (current_event.param2 > (ROWS - 3)) {
          current_event.eventType = KEYSTROKE;
          current_event.param1 = ESCAPE_KEY;
        } else {
          virtual_keyboard = true;
          start_text_input();
        }
      } else {
        current_event.eventType = MOUSE_UP;
      }
      if (!virtual_keyboard) {
        stop_text_input();
      }
      break;
    case SDL_FINGERMOTION: // For long press check
      if (!SDL_TICKS_PASSED(SDL_GetTicks(),
                            zoom_changed_time + ZOOM_CHANGED_INTERVAL)) {
        current_event.eventType = EVENT_ERROR;
        break;
      }
      if (finger_down_time != 0 &&
          SDL_TICKS_PASSED(SDL_GetTicks(),
                           finger_down_time + long_press_interval)) {
        if (on_dpad && allow_dpad_mode_change) {
          dpad_mode = !dpad_mode;
          screen_changed = true;
          pauseForMilliseconds(0);
        }
        finger_down_time = 0;
      }
      break;
    case SDL_MULTIGESTURE:
      if (event.mgesture.numFingers == 2 && game_started) {
        zoom_level *= (1.0 + event.mgesture.dDist * 3);
        zoom_level = max(1.0, min(zoom_level, max_zoom));
        zoom_changed_time = SDL_GetTicks();
        screen_changed = true;
        if (SDL_TICKS_PASSED(SDL_GetTicks(),
                             zoom_toggled_time + ZOOM_TOGGLED_INTERVAL)) {
          zoom_toggle = true;
          zoom_toggled_time = 0;
        }
      }
      break;
    case SDL_KEYDOWN:
      current_event.eventType = KEYSTROKE;
      SDL_KeyCode k = event.key.keysym.sym;
      if (event.key.keysym.mod & KMOD_CTRL) {
        current_event.controlKey = ctrl_pressed = true;
        ctrl_time = SDL_GetTicks();
      }
      switch (k) {
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
        if (event.key.keysym.mod & (KMOD_SHIFT | KMOD_CAPS)) {
          if ('a' <= k && k <= 'z') {
            k += 'A' - 'a';
            current_event.shiftKey = true;
          } else {
            k += '?' - '/';
          }
        }
        current_event.param1 = k;
        break;
      }
      break;
    }
  }
  if (SDL_TICKS_PASSED(SDL_GetTicks(), ctrl_time + 1000)) {
    // Wait for 1 second to disable CTRL after pressing and another input
    current_event.controlKey = ctrl_pressed = false;
  }
  boolean dialog_popup = !smart_zoom_allowed();
  if (in_left_panel || dialog_popup) {
    if (prev_zoom_toggle == unset) {
      prev_zoom_toggle = zoom_toggle ? set_true : set_false;
    }
    zoom_toggle = false;
  } else if (prev_zoom_toggle != unset) {
    zoom_toggle = prev_zoom_toggle == set_true ? true : false;
    prev_zoom_toggle = unset;
  }
  return current_event.eventType != EVENT_ERROR;
}
