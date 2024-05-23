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

ncw_err
ncw_scroll_window_init(scroll_window_t *sw, int x, int y, int width, int height, const char *title);

ncw_err
ncw_scroll_window_close(scroll_window_t *sw);

ncw_err
ncw_scroll_window_add_line(scroll_window_t sw, const char *line);

// Internal
ncw_err
scroll_window_update(void *window_ctx);

#endif // NCW_SCROLL_H_HEADER_GUARD
