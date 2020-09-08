#ifndef _input_h_
#define _input_h_

#include "Rogue.h"

extern rogueEvent current_event;

boolean process_events();

extern boolean ctrl_pressed;
extern boolean requires_text_input;
extern boolean virtual_keyboard_active;

void start_text_input();

void stop_text_input();

#endif
