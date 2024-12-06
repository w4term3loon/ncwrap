#include <stdlib.h>
#include <string.h>

#include "ncurses.h"
#include "ncwrap.h"

#include "helper.h"
#include "scroll.h"
#include "window.h"

ncw_err
scroll_window_update(void *window_ctx);

ncw_err
ncw_scroll_window_init(scroll_window_t *sw, int x, int y, int width, int height,
                       const char *title) {

  ncw_err err = NCW_OK;
  if (NULL == title || 2 > width || 2 > height || NULL != *sw) {
    err = NCW_INVALID_PARAM;
    goto _end;
  }

  // Scroll window handle
  *sw = (scroll_window_t)malloc(sizeof(struct scroll_window));
  if (NULL == *sw) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _end;
  }

  // Settings
  (*sw)->width = width;
  (*sw)->height = height;
  (*sw)->next_line = NULL;

  // Title
  (*sw)->title = (char *)malloc(strlen(title) + 1);
  if (NULL != (*sw)->title) {
    (void)safe_strncpy((*sw)->title, title, strlen(title) + 1);
  } else {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _handle;
  }

  // Ncurses window
  (*sw)->window = newwin(height, width, y, x);
  if (NULL == (*sw)->window) {
    err = NCW_NCURSES_FAIL;
    goto _title;
  }

  // Event functions
  struct update_t update = {.cb = scroll_window_update, .ctx = (void *)sw};
  struct event_handler_t handler = {.cb = NULL, .ctx = NULL};
  (*sw)->wh = window_register(update, handler);
  if (NULL == (*sw)->wh) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _window;
  }

  // enable scrolling in this window
  scrollok((*sw)->window, TRUE);

_end:
  return err;
_window:
  delwin((*sw)->window);
_title:
  free((*sw)->title);
_handle:
  free((void *)*sw);
  *sw = NULL;
  goto _end;
}

ncw_err
ncw_scroll_window_close(scroll_window_t *sw) {

  ncw_err err = NCW_OK;
  if (NULL == *sw) {
    err = NCW_INVALID_PARAM;
    goto _end;
  }

  // NOTE: this may not be the best solution
  // since tis loses the handle for the memory
  if (OK != window_clear((*sw)->window) || OK != delwin((*sw)->window)) {
    err = NCW_NCURSES_FAIL;
    // fallthrough
  }

  window_unregister((*sw)->wh);

  free((void *)(*sw)->next_line);
  free((void *)(*sw)->title);
  free((void *)*sw);
  *sw = NULL;

_end:
  return err;
}

ncw_err
ncw_scroll_window_add_line(scroll_window_t sw, const char *line) {

  if (NULL != sw->next_line) {
    free(sw->next_line);
  }

  sw->next_line = (char *)malloc(strlen(line) + 1);
  if (NULL != sw->next_line) {
    (void)safe_strncpy(sw->next_line, line, strlen(line) + 1);
  } else {
    return NCW_INSUFFICIENT_MEMORY;
  }

  return NCW_OK;
}

ncw_err
scroll_window_update(void *window_ctx) {

  ncw_err err = NCW_OK;
  scroll_window_t sw = *((scroll_window_t *)window_ctx);

  // if there is no new next line, skip
  if (NULL == sw->next_line) {
    goto _skip;
  }

  // displace all lines 1 up (literally)
  if (OK != scroll(sw->window)) {
    goto _fail;
  }

  // clear the place of the added line
  if (OK != wmove(sw->window, sw->height - 2, 1)) {
    goto _fail;
  }

  if (OK != wclrtoeol(sw->window)) {
    goto _fail;
  }

  // print the line in the correct place
  if (OK != mvwprintw(sw->window, sw->height - 2, 1, "%s", sw->next_line)) {
    goto _fail;
  }

  // free next line for new one
  free((void *)sw->next_line);
  sw->next_line = NULL;

_skip: //< reconstruct the widnow box
  if (OK != window_draw_box(sw->window, sw->title)) {
    goto _fail;
  }

_end:
  return err;
_fail:
  err = NCW_NCURSES_FAIL;
  goto _end;
}

