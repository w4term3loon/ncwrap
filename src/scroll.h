#ifndef NCW_SCROLL_H_HEADER_GUARD
#define NCW_SCROLL_H_HEADER_GUARD

#include "ncwrap.h"
#include <ncurses.h>

struct scroll_window {
  char *title;
  WINDOW *window;
  window_handle_t wh;
  int width, height;
  char *next_line;
};

#endif // NCW_SCROLL_H_HEADER_GUARD
