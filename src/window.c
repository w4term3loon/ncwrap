#include <stdlib.h>

#include "window.h"

static window_handle_t g_wh = NULL;

window_handle_t
get_window_handle(void) {
  return g_wh;
}

void
set_window_handle(window_handle_t wh) {
  g_wh = wh;
}

window_handle_t
window_register(struct update_t update, struct event_handler_t handler) {

  // malloc new element
  window_handle_t new = (window_handle_t)malloc(sizeof(struct window_handle));
  if (NULL == new) {
    return (window_handle_t)NULL;
  }

  // init new element
  new->update = update;
  new->event_handler = handler;

  // if empty
  if (NULL == g_wh) {
    // if first in g_wh
    new->prev = new;
    new->next = new;
    g_wh = new;
  } else {
    // if not first init prev
    g_wh->prev->next = new;
    new->prev = g_wh->prev;

    g_wh->prev = new;
    new->next = g_wh;
  }

  return new;
}

void
window_unregister(window_handle_t wh) {

  // invalid pointer
  if (NULL == wh) {
    return;
  }

  // g_wh points here
  if (g_wh == wh) {

    // this is the last element
    if (g_wh == wh->prev && wh->prev == wh->next) {
      g_wh = NULL;
      goto clean;

    } else {
      g_wh = wh->next;
    }
  }

  // g_wh points somewhere else
  wh->prev->next = wh->next;
  wh->next->prev = wh->prev;

clean:
  free(wh);
}

