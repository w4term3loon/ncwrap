#ifndef NCW_MENU_H_HEADER_GUARD
#define NCW_MENU_H_HEADER_GUARD

#include "ncwrap.h"
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

ncw_err
ncw_menu_window_init(menu_window_t *mw, int x, int y, int width, int height, const char *title);

ncw_err
ncw_menu_window_close(menu_window_t *mw);

ncw_err
ncw_menu_window_add_option(menu_window_t mw, const char *label, void (*cb)(void *), void *ctx);

ncw_err
ncw_menu_window_delete_option(menu_window_t mw, const char *label);

// Internal
ncw_err
menu_window_update(void *window_ctx);

ncw_err
menu_window_handler(int event, void *window_ctx);

#endif // NCW_MENU_H_HEADER_GUARD
