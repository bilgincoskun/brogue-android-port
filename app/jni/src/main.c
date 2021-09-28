#include "SDL.h"
#include "SDL_ttf.h"
#include "config.h"
#include "display.h"
#include "input.h"
#include "platform.h"
#include <errno.h>
#include <jni.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "IncludeGlobals.h"

#define MAX_ERROR_LENGTH 200
#define FRAME_INTERVAL 50

#define DAY_TO_TIMESTAMP 86400

struct brogueConsole currentConsole;

static SDL_Window *window;
static SDL_Rect screen;
static _Atomic boolean resumed = false;

boolean hasGraphics = true;

enum graphicsModes graphicsMode = TEXT_GRAPHICS;

void destroy_assets() {
  SDL_SetRenderTarget(renderer, NULL);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void general_error(boolean critical,const char *error_title, const char *error_message, ...) {
  char buffer[MAX_ERROR_LENGTH];
  va_list a;
  va_start(a, error_message);
  vsnprintf(buffer, MAX_ERROR_LENGTH, error_message, a);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, error_title, buffer, NULL);
  va_end(a);
  if(critical) {
    destroy_assets();
    destroy_font();
    TTF_Quit();
    SDL_Quit();
    exit(-1);
  }
}

#define invalid_config_error(error_title,...) general_error(true,error_title,__VA_ARGS__)

void create_assets() {
  if (SDL_CreateWindowAndRenderer(display.w, display.h,
                                  SDL_WINDOW_FULLSCREEN |
                                      SDL_WINDOW_ALWAYS_ON_TOP,
                                  &window, &renderer)) {
    invalid_config_error("SDL Error", "Couldn't create window and renderer: %s",
                         SDL_GetError());
  }
  screen_texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, display.w, display.h);
  SDL_SetRenderTarget(renderer, screen_texture);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, COLOR_MAX);
  SDL_RenderClear(renderer);
  if (dpad_enabled) {
    SDL_Surface *dpad_i = SDL_LoadBMP("dpad.bmp");
    dpad_image_select = SDL_CreateTextureFromSurface(renderer, dpad_i);
    SDL_SetTextureAlphaMod(dpad_image_select, dpad_transparency);
    dpad_image_move = SDL_CreateTextureFromSurface(renderer, dpad_i);
    SDL_SetTextureColorMod(dpad_image_move, COLOR_MAX, COLOR_MAX, 155);
    SDL_SetTextureAlphaMod(dpad_image_move, dpad_transparency);
    SDL_FreeSurface(dpad_i);
    double area_width = min(cell_w * (LEFT_PANEL_WIDTH - 4), cell_h * 20);
    dpad_area.h = dpad_area.w = (dpad_width) ? dpad_width : area_width;
    dpad_area.x = (dpad_x_pos) ? dpad_x_pos : 3 * cell_w;
    dpad_area.y =
        (dpad_y_pos) ? dpad_y_pos : (display.h - (area_width + 2 * cell_h));
  }
  SDL_Surface *settings_surface = SDL_LoadBMP("settings.bmp");
  settings_image = SDL_CreateTextureFromSurface(renderer, settings_surface);
  SDL_SetTextureAlphaMod(settings_image, COLOR_MAX / 3);
  SDL_FreeSurface(settings_surface);
  settings_icon_area.h = settings_icon_area.w =
      min(cell_w * (LEFT_PANEL_WIDTH - 4), cell_h * 20);
  settings_icon_area.x = 2 * cell_w;
  settings_icon_area.y = (display.h - (settings_icon_area.h + 2 * cell_h));

  if (keyboard_visibility == 2) {
    start_text_input();
  }
  init_glyphs();
}

uint8_t convert_color(short c) {
  c = c * COLOR_MAX / 100;
  return max(0, min(c, COLOR_MAX));
}

int suspend_resume_filter(void *userdata, SDL_Event *event) {
  switch (event->type) {
  case SDL_APP_WILLENTERBACKGROUND:
    return 0;
  case SDL_APP_WILLENTERFOREGROUND:
    resumed = true;
    return 0;
  }
  return 1;
}

void TouchScreenGameLoop() {
  restart_game = true;
  settings_changed = false;
  do {
    if (restart_game) {
      display = screen;
      if (force_portrait) {
        SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS,
                                "Portrait PortraitUpsideDown",
                                SDL_HINT_OVERRIDE);
        int tmp = display.w;
        display.w = display.h;
        display.h = tmp;
      } else {
        SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS,
                                "LandscapeLeft LandscapeRight",
                                SDL_HINT_OVERRIDE);
      }
      char render_hint[2] = {filter_mode + '0', 0};
      SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, render_hint,
                              SDL_HINT_OVERRIDE);
      if (custom_cell_width != 0) {
        cell_w = custom_cell_width;
      } else {
        cell_w = ((double)display.w) / COLS;
      }
      if (custom_cell_height != 0) {
        cell_h = custom_cell_height;
      } else {
        cell_h = ((double)display.h) / ROWS;
      }
      left_panel_box = (SDL_Rect){
          .x = 0, .y = 0, .w = LEFT_PANEL_WIDTH * cell_w, .h = ROWS * cell_h};
      log_panel_box = (SDL_Rect){.x = LEFT_PANEL_WIDTH * cell_w,
                                 .y = 0,
                                 .w = (COLS - LEFT_PANEL_WIDTH) * cell_w,
                                 .h = TOP_LOG_HEIGIHT * cell_h};
      button_panel_box =
          (SDL_Rect){.x = LEFT_PANEL_WIDTH * cell_w,
                     .y = (ROWS - BOTTOM_BUTTONS_HEIGHT) * cell_h,
                     .w = (COLS - LEFT_PANEL_WIDTH) * cell_w,
                     .h = BOTTOM_BUTTONS_HEIGHT * cell_h};
      grid_box = grid_box_zoomed = (SDL_Rect){
          .x = LEFT_PANEL_WIDTH * cell_w,
          .y = TOP_LOG_HEIGIHT * cell_h,
          .w = (COLS - LEFT_PANEL_WIDTH) * cell_w,
          .h = (ROWS - TOP_LOG_HEIGIHT - BOTTOM_BUTTONS_HEIGHT) * cell_h};
      create_assets();
      if (!init_font()) {
        invalid_config_error(
            "Font Error",
            "Resolution/cell size is too small for minimum allowed font size");
      }
    } else if (settings_changed) {
      create_assets();
    }
    settings_changed = restart_game = false;
    rogue.nextGame = NG_NOTHING;
    rogue.nextGamePath[0] = '\0';
    rogue.nextGameSeed = 0;
    rogueMain();
    destroy_assets();
  } while (settings_changed || restart_game);
}

boolean resume(){
  if (resumed) {
    resumed = false;
    destroy_assets();
    create_assets();
    refreshScreen();
    return true;
  }
  return false;
}

boolean TouchScreenPauseForMilliseconds(short milliseconds) {
  uint32_t init_time = SDL_GetTicks();
  draw_screen();
  uint32_t epoch = SDL_GetTicks() - init_time;
  if (epoch < milliseconds) {
    SDL_Delay(milliseconds - epoch);
  }
  resume();
  return process_events();
}

void TouchScreenNextKeyOrMouseEvent(rogueEvent *returnEvent, boolean textInput,
                                    boolean colorsDance) {
  resume();
  while (!process_events()) {
    refresh_animations(colorsDance);
    TouchScreenPauseForMilliseconds(FRAME_INTERVAL);
  }
  *returnEvent = current_event;
  current_event.eventType = EVENT_ERROR; // unset the event
}

void TouchScreenPlotChar(enum displayGlyph ch, short xLoc, short yLoc,
                         short foreRed, short foreGreen, short foreBlue,
                         short backRed, short backGreen, short backBlue) {

  SDL_FRect rect;
  rect.x = xLoc * cell_w;
  rect.y = yLoc * cell_h;
  rect.w = cell_w;
  rect.h = cell_h;
  SDL_SetRenderDrawColor(renderer, convert_color(backRed),
                         convert_color(backGreen), convert_color(backBlue),
                         COLOR_MAX);
  SDL_RenderFillRectF(renderer, &rect);
  draw_glyph(ch, rect, convert_color(foreRed), convert_color(foreGreen),
             convert_color(foreBlue));
  screen_changed = true;
}

void TouchScreenRemap(const char *input_name, const char *output_name) {}

boolean TouchScreenModifierHeld(int modifier) {
  // Shift key check is unnecessary since function is not called with modifier
  // == 0 in anywhere
  return modifier == 1 && ctrl_pressed;
}

static enum graphicsModes TouchScreenSetGraphicsMode(enum graphicsModes mode){
  graphicsMode = mode;
  refreshScreen();
  return mode;
}

void TouchScreenTextInputStart() {
  if (!virtual_keyboard_active) { // Open keyboard if it is not opened by demand
                                  // previously
    requires_text_input = true;
    start_text_input();
  }
}

void TouchScreenTextInputStop() {
  if (requires_text_input) {
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
    .setGraphicsMode = TouchScreenSetGraphicsMode,
    .textInputStart = TouchScreenTextInputStart,
    .textInputStop = TouchScreenTextInputStop,
};

boolean git_version_check(JNIEnv *env, jobject activity, jclass cls) {
  if (ask_for_update_check) {
    const SDL_MessageBoxButtonData buttons[] = {
        {0, 0, "Later"},
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
  jmethodID method_id =
      (*env)->GetMethodID(env, cls, "gitVersionCheck", "()Ljava/lang/String;");
  jstring ver_ = (*env)->CallObjectMethod(env, activity, method_id);
  const char *ver = (*env)->GetStringUTFChars(env, ver_, NULL);
  char version_message[500];
  char error_title[] = "Cannot Get New Version Info";
  boolean return_value = false;
  switch (ver[0]) {
  case ' ': // No Error
    if (strlen(ver) > 1) {
      sprintf(version_message,
              "Latest version %s released.Opening download page now", ver);
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "New Version Found",
                               version_message, NULL);
      jmethodID method_id =
          (*env)->GetMethodID(env, cls, "openDownloadLink", "()V");
      (*env)->CallVoidMethod(env, activity, method_id);
    } else if (ask_for_update_check) {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "No new Version",
                               "Already using latest version", NULL);
    }
    return_value = true;
    break;
  case '1': // Timeout Error
    if (ask_for_update_check)
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, error_title,
                               "Connection timed out", NULL);
    break;
  case '2': // JSON Error

    if (ask_for_update_check)
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, error_title,
                               "Cannot parse json", NULL);
    break;
  case '3': // Connection Error
    if (ask_for_update_check)
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, error_title,
                               "Cannot download version info", NULL);
    break;
  }
  (*env)->ReleaseStringUTFChars(env, ver_, ver);
  return return_value;
}

void open_manual(JNIEnv *env, jobject activity, jclass cls) {
  const SDL_MessageBoxButtonData buttons[] = {
      {0, 0, "No"},
      {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes"},
  };

  const SDL_MessageBoxData messageboxdata = {
      SDL_MESSAGEBOX_INFORMATION,
      NULL,
      "Manual",
      "Manual contains information about control, settings etc. Do you want to "
      "read it now?",
      SDL_arraysize(buttons),
      buttons,
      NULL,
  };
  int buttonid;
  SDL_ShowMessageBox(&messageboxdata, &buttonid);
  if (buttonid == 0) {
    return;
  }
  jmethodID method_id = (*env)->GetMethodID(env, cls, "openManual", "()V");
  (*env)->CallVoidMethod(env, activity, method_id);
}

void config_folder(JNIEnv *env, jobject activity, jclass cls,
                   boolean first_run) {
  jmethodID method_id =
      (*env)->GetMethodID(env, cls, "configFolder", "()Ljava/lang/String;");
  jstring folder_ = (*env)->CallObjectMethod(env, activity, method_id);
  if (folder_ != NULL) {
    const char *folder = (*env)->GetStringUTFChars(env, folder_, NULL);
    chdir(folder);
    (*env)->ReleaseStringUTFChars(env, folder_, folder);
  } else {
    if (first_run) {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Config Folder Error",
                               "Cannot create or access sdcard/Brogue."
                               "Will save to data folder of the app",
                               NULL);
    }
    chdir(SDL_AndroidGetExternalStoragePath());
  }
}

int brogue_main(void *data) {
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
  FILE *fc;
  JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
  jobject activity = (jobject)SDL_AndroidGetActivity();
  jclass cls = (*env)->GetObjectClass(env, activity);
  jmethodID method_id;
  boolean first_run = false;
  if (access("first_run", F_OK) == -1) {
    method_id = (*env)->GetMethodID(env, cls, "needsWritePermission", "()Z");
    if ((*env)->CallBooleanMethod(env, activity, method_id)) {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Write Permission",
                               "To use sdcard/Brogue as save folder you need "
                               "to grant app write permission in "
                               "Android 6.0+. Otherwise it will save the app "
                               "will use the folder under Android/data",
                               NULL);
      method_id = (*env)->GetMethodID(env, cls, "grantPermission", "()V");
      (*env)->CallVoidMethod(env, activity, method_id);
      SDL_Delay(1000);
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Continue?", "",
                               NULL);
    }
    open_manual(env, activity, cls);
    fc = fopen("first_run", "w");
    fclose(fc);
    first_run = true;
  }

  time_t update_check_time = 0;
  if (access("last_update_check", F_OK) == -1) {
    fc = fopen("last_update_check", "w");
  } else {
    fc = fopen("last_update_check", "r");
    fscanf(fc, "%ld", &update_check_time);
    fclose(fc);
    fc = fopen("last_update_check", "w");
  }
  config_folder(env, activity, cls, first_run);
  set_conf("", ""); // set default values of config
  load_conf();
  switch(default_graphics_mode){
      case 0:
        graphicsMode = TEXT_GRAPHICS;
        break;
      case 1:
        graphicsMode = TILES_GRAPHICS;
        break;
      case 2:
        graphicsMode = HYBRID_GRAPHICS;
        break;

  }
  if (check_update) {
    time_t new_time;
    time(&new_time);
    if (new_time > update_check_time) {
      if ((new_time - update_check_time) >
              DAY_TO_TIMESTAMP * check_update_interval &&
          git_version_check(env, activity, cls)) {
        update_check_time = new_time;
      }
    }
  }
  fprintf(fc, "%ld", update_check_time);
  fclose(fc);
  (*env)->DeleteLocalRef(env, activity);
  (*env)->DeleteLocalRef(env, cls);
  if (chdir(BROGUE_VERSION_STRING) == -1) {
    if (errno != ENOENT) {
      invalid_config_error("Save Folder Error",
                           "Cannot create/enter the save folder");
    }
    if (mkdir(BROGUE_VERSION_STRING, 0770) || chdir(BROGUE_VERSION_STRING)) {
      invalid_config_error("Save Folder Error",
                           "Cannot create/enter the save folder");
    }
  }
  SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    invalid_config_error("SDL Error", "Unable to initialize SDL: %s", SDL_GetError());
  }
  if (TTF_Init() != 0) {
    invalid_config_error("SDL_ttf Error", "Unable to initialize SDL_ttf: %s",
                         SDL_GetError());
  }
  if (SDL_GetDisplayBounds(0, &screen) != 0) {
    invalid_config_error("SDL Error", "SDL_GetDesktopDisplayMode failed: %s",
                         SDL_GetError());
  }
  SDL_SetEventFilter(suspend_resume_filter, NULL);
  SDL_Thread *thread = SDL_CreateThreadWithStackSize(brogue_main, "Brogue",
                                                     8 * 1024 * 1024, NULL);
  if (thread != NULL) {
    int result;
    SDL_WaitThread(thread, &result);
  } else {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_WARNING, "Thread Error",
        "Cannot create a thread with sufficient stack size. "
        "The game will start nonetheless but be aware that some seeds may "
        "cause the game "
        "to crash in this mode.",
        NULL);
    brogue_main(NULL);
  }
  destroy_font();
  TTF_Quit();
  SDL_Quit();
  free(setting_list);
  exit(0); // return causes problems
}
