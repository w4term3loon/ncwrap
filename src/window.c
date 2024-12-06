#include <assert.h>
#include <stdlib.h>

#include "helper.h"
#include "window.h"

// The global handle for the window buffer
// that specifies which window receives the
// events at any moment
static window_handle_t g_focus;

window_handle_t
get_focus(void) {
  return g_focus;
}

void
set_focus(window_handle_t wh) {

  if (NULL == wh) {
    fprintf(stderr, "empty window handler");
    return;
  }

  // notify the last handler
  if (g_focus != NULL && g_focus->event_handler.cb != NULL) {
    g_focus->event_handler.cb(FOCUS_OFF, g_focus->event_handler.ctx);
  }

  g_focus = wh;

  // notify the next handler
  g_focus->event_handler.cb(FOCUS_ON, g_focus->event_handler.ctx);
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
  if (NULL == g_focus) {
    // if first window
    new->prev = new;
    new->next = new;
    g_focus = new;

    // notify the event handler of the window
    g_focus->event_handler.cb(FOCUS_ON, g_focus->event_handler.ctx);
  } else {
    // init last facing side
    g_focus->prev->next = new;
    new->prev = g_focus->prev;

    // init first  facing side
    g_focus->prev = new;
    new->next = g_focus;

    // if the focused window is non-interactive
    if (g_focus->event_handler.cb == NULL) {
      ncw_focus_step();
    }
  }

  return new;
}

void
window_unregister(window_handle_t wh) {

  // invalid pointer
  if (NULL == wh) {
    return;
  }

  // g_focus points here
  if (g_focus == wh) {

    // this is the last element
    if (g_focus == wh->prev && wh->prev == wh->next) {
      g_focus = NULL;
      goto clean;

    } else {
      g_focus = wh->next;
    }
  }

  // g_focus points somewhere else
  wh->prev->next = wh->next;
  wh->next->prev = wh->prev;

clean:
  free(wh);
}
