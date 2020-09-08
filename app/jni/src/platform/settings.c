#include "SDL.h"
#include "config.h"
#include "display.h"

extern cellDisplayBuffer displayBuffer[COLS][ROWS];

void resume();

void to_buffer(uchar ch, short xLoc, short yLoc, short foreRed, short foreGreen,
               short foreBlue, short backRed, short backGreen, short backBlue) {
  displayBuffer[xLoc][yLoc] = (struct cellDisplayBuffer){
      .character = ch,
      .foreColorComponents = {foreRed, foreGreen, foreBlue},
      .backColorComponents = {backRed, backGreen, backBlue},
      .needsUpdate = true};
}

void redraw_value(int index) {
  setting *s = &setting_list[index];
  int start = s->xLoc + SETTING_NAME_MAX_LEN;
  char buffer[SETTING_VALUE_MAX_LEN - 2] = {0};
  switch (s->t) {
  case boolean_:
    strcpy(buffer, s->new.b ? "true " : "false");
    break;
  case int_:
    sprintf(buffer, "%d", s->new.i);
    break;
  case double_:
    sprintf(buffer, "%.1f", s->new.d);
    break;
  case section_:
  case button_:
    break;
  }
  if (s->t != section_) {
    to_buffer('<', start, s->yLoc, 0, 0, 0, 60, 60, 60);
    to_buffer('>', start + SETTING_VALUE_MAX_LEN - 1, s->yLoc, 0, 0, 0, 60, 60,
              60);
    for (int i = 0; i < SETTING_VALUE_MAX_LEN - 2; i++) {
      to_buffer(' ', i + start + 1, s->yLoc, 0, 0, 0, 100, 100, 100);
    }
    int offset = (SETTING_VALUE_MAX_LEN - 2 - strlen(buffer)) / 2;
    for (int i = 0; buffer[i]; i++) {
      to_buffer(buffer[i], i + start + 1 + offset, s->yLoc, 0, 0, 0, 100, 100,
                100);
    }
  }
}

void rebuild_settings_menu(int current_section) { //-1 means no section is open
  clearDisplayBuffer(displayBuffer);
  short xLoc = 3, yLoc = 1;
  int section_no = 0;
  for (int i = 0; i < setting_len; i++) {
    if (setting_list[i].t == section_) {
      section_no += 1;
    } else if (setting_list[i].t != button_ &&
               section_no != (current_section + 1)) {
      setting_list[i].yLoc = -1;
      continue;
    }
    if (setting_list[i].t != button_) {
      setting_list[i].xLoc = xLoc;
      setting_list[i].yLoc = yLoc;
    }
    redraw_value(i);
    char *name = setting_list[i].name;
    char bg = 0, fg = 100;
    if (setting_list[i].t == section_ || setting_list[i].t == button_) {
      bg = 100;
      fg = 0;
    }

    for (int j = 0; name[j]; j++) {
      to_buffer(name[j] != '_' ? name[j] : ' ', setting_list[i].xLoc + j,
                setting_list[i].yLoc, fg, fg, fg, bg, bg, bg);
    }
    if (setting_list[i].t != button_) {
      yLoc += 3;
      if (yLoc >= ROWS - 2) {
        xLoc += SETTING_NAME_MAX_LEN + SETTING_VALUE_MAX_LEN + 2;
        yLoc = 1;
      }
    }
  }
  refreshScreen();
  if (screen_changed) {
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_SetRenderTarget(renderer, screen_texture);
  }
}

boolean restart_game = false;
boolean settings_changed = false;

void settings_menu() {
  restart_game = true;
  settings_changed = false;
  int current_section = 0;
  int hold = 0;
  int acc = 0;
  int16_t cursor_x, cursor_y;
  for (int i = 0; i < setting_len; i++) {
    setting *s = &setting_list[i];
    switch (s->t) {
    case boolean_:
      s->new.b = *(boolean *)s->value;
      break;
    case int_:
      s->new.i = *(int *)s->value;
      break;
    case double_:
      s->new.d = *(double *)s->value;
      break;
    case section_:
    case button_:
      break;
    }
  }
  rebuild_settings_menu(current_section);
  while (true) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_FINGERDOWN) {
        double raw_input_x, raw_input_y;
        raw_input_x = event.tfinger.x * display.w;
        raw_input_y = event.tfinger.y * display.h;
        cursor_x = min(COLS - 1, raw_input_x / cell_w);
        cursor_y = min(ROWS - 1, raw_input_y / cell_h);
        hold = SDL_GetTicks();

      } else if (event.type == SDL_FINGERUP) {
        hold = 0;
      }
    }
    if (hold) {
      acc += 1;
      for (int i = 0; i < setting_len; i++) {
        setting *s = &setting_list[i];
        if (abs(s->yLoc - cursor_y) <= 1 &&
            (s->xLoc <= cursor_x && cursor_x <= s->xLoc + SETTING_NAME_MAX_LEN +
                                                    SETTING_VALUE_MAX_LEN)) {
          boolean decrease =
              abs(cursor_x - s->xLoc - SETTING_NAME_MAX_LEN) <= 2;
          boolean increase = cursor_x >= s->xLoc + SETTING_NAME_MAX_LEN +
                                             SETTING_VALUE_MAX_LEN - 2;
          boolean menu_changed = false;
          FILE *st;
          switch (s->t) {
          case section_:
            current_section = s->default_.s;
            cursor_x = -1;
            menu_changed = true;
            break;
          case button_:
            switch (s->default_.id) {
            case DEFAULTS_BUTTON_ID:
              for (int i = 0; i < setting_len; i++) {
                setting *s = &setting_list[i];
                s->new = s->default_;
              }
              menu_changed = true;
              break;
            case CANCEL_BUTTON_ID:
              return;
            case OK_BUTTON_ID:
              st = fopen("../" SETTINGS_FILE, "w");
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
              return;
            }
            break;
          case boolean_:
            if (increase || decrease) {
              s->new.b = !s->new.b;
              menu_changed = true;
            }
            break;
          case int_:
            if (decrease) {
              s->new.i = max(s->min_.i, s->new.i - 1);
              menu_changed = true;
            } else if (increase) {
              s->new.i = min(s->max_.i, s->new.i + 1);
              menu_changed = true;
            }
            break;
          case double_:
            s->new.d = floorf(s->new.d * 10) / 10;
            if (decrease) {
              s->new.d = max(s->min_.d, s->new.d - 0.1);
              menu_changed = true;
            } else if (increase) {
              s->new.d = min(s->max_.d, s->new.d + 0.1);
              menu_changed = true;
            }
            break;
          }
          if (menu_changed) {
            rebuild_settings_menu(current_section);
          }
        }
      }

    } else {
      acc = 0;
    }
    SDL_Delay(100 / max(1, (acc - 10) * 2));
  }
}
