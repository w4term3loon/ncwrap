#ifndef NCWRAP_IMPL_H_HEADER_GUARD
#define NCWRAP_IMPL_H_HEADER_GUARD

typedef ncw_err (*update_cb)(void *window_ctx);
typedef struct {
  update_cb cb;
  void *ctx;
} update_t;

typedef ncw_err (*handler_cb)(int event, void *window_ctx);
typedef struct {
  handler_cb cb;
  void *ctx;
} event_handler_t;

struct window_handle {
  update_t update;
  event_handler_t event_handler;
  window_handle_t next;
  window_handle_t prev;
};

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

struct scroll_window {
  char *title;
  WINDOW *window;
  window_handle_t wh;
  int width, height;
  char *next_line;
};

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

#endif // NCWRAP_IMPL_H_HEADER_GUARD
