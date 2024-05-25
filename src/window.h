#ifndef NCW_WINDOW_H_HEADER_GUARD
#define NCW_WINDOW_H_HEADER_GUARD

#include "ncwrap.h"

typedef ncw_err (*update_cb)(void *window_ctx);
struct update_t {
  update_cb cb;
  void *ctx;
};

typedef ncw_err (*handler_cb)(int event, void *window_ctx);
struct event_handler_t {
  handler_cb cb;
  void *ctx;
};

struct window_handle {
  struct update_t update;
  struct event_handler_t event_handler;
  window_handle_t next;
  window_handle_t prev;
};

window_handle_t
window_register(struct update_t update, struct event_handler_t handler);

void
window_unregister(window_handle_t wh);

window_handle_t
get_focus(void);

void
set_focus(window_handle_t);

#endif // NCW_WINDOW_H_HEADER_GUARD
