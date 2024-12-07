#ifndef NCW_INPUT_H_HEADER_GURARD
#define NCW_INPUT_H_HEADER_GURARD

#include "ncurses.h"
#include "window.h"
#include "ncwrap.h"

struct input_window {
  char is_popup;
  char *title;
  WINDOW *window;
  window_handle_t wh;
  int width;
  output_cb cb;
  void *ctx;
  char *buf, *display;
  size_t buf_sz, line_sz;
  int display_offs, cursor_offs;
  int focus;
};

#endif // NCW_INPUT_H_HEADER_GURARD
