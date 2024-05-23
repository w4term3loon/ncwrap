#ifndef NCW_INPUT_H_HEADER_GURARD
#define NCW_INPUT_H_HEADER_GURARD

#include "ncurses.h"
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

ncw_err
ncw_input_window_init(input_window_t *iw, int x, int y, int width, const char *title,
                      char is_popup);

ncw_err
ncw_input_window_close(input_window_t *iw);

ncw_err
ncw_input_window_set_output(input_window_t iw, output_cb cb, void *ctx);

// Internal
ncw_err
input_window_update(void *window_ctx);

ncw_err
input_window_handler(int event, void *window_ctx);

#endif // NCW_INPUT_H_HEADER_GURARD
