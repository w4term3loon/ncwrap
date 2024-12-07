#ifndef NCW_MENU_H_HEADER_GUARD
#define NCW_MENU_H_HEADER_GUARD

#include "window.h"
#include "ncurses.h"

typedef struct {
  char *label;
  void (*cb)(void *);
  void *ctx;
} option_t;

struct menu_window {
  char *title;
  WINDOW *window;
  window_handle_t wh;
  int width, height;
  option_t *options;
  int options_num;
  int highlight;
  int highlight_buf;
};

#endif // NCW_MENU_H_HEADER_GUARD
