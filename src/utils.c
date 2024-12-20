#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ncurses.h"
#include "ncwrap.h"

#include "window.h"

ncw_err
ncw_init(void) {

  ncw_err err = NCW_OK;

  /* init ncurses library */
  stdscr = initscr();
  if (NULL == stdscr) {
    goto fail;
  }

  /* refresh the main screen */
  if (ERR == refresh()) {
    goto fail;
  }

  /* tell ncurses not to do NL->CR/NL on output */
  if (OK != nonl()) {
    goto fail;
  }

  /* take input chars one at a time, no wait for '\n' */
  if (OK != cbreak()) {
    goto fail;
  }

  /* set cursor to invisible  */
  if (ERR == curs_set(0)) {
    goto fail;
  }

  /* disble echo to windows */
  if (OK != noecho()) {
    goto fail;
  }

  if (OK != keypad(stdscr, TRUE)) {
    goto fail;
  }

  wtimeout(stdscr, 100);

end:
  return err;

fail:
  err = NCW_NCURSES_FAIL;
  goto end;
}

ncw_err
ncw_close(void) {

  ncw_err err = NCW_OK;

  /* terminate ncurses library */
  if (OK != endwin()) {
    goto fail;
  }

end:
  return err;

fail:
  err = NCW_NCURSES_FAIL;
  goto end;
}

void
ncw_update(void) {

  window_handle_t focus = get_focus();
  if (NULL == focus) {
    return;
  }

  // update get_window_handle() last
  for (window_handle_t iter = focus->next;; iter = iter->next) {
    assert(iter != NULL && iter->update.cb != NULL);
    iter->update.cb(iter->update.ctx);
    if (focus == iter) {
      break;
    }
  }

  // print to scr
  (void)doupdate();

  return;
}

void
ncw_focus_step(void) {

  if (NULL == get_focus()) {
    fprintf(stderr, "no interactive windows\n");
    return;
  }

  // find the next handler
  window_handle_t iter = get_focus()->next;
  while (NULL == iter->event_handler.cb) {
    iter = iter->next;
  }

  // set the next handler
  set_focus(iter);
}

void
ncw_event_handler(int event) {
  window_handle_t wh = get_focus();
  if (wh != NULL && wh->event_handler.cb != NULL) {
    wh->event_handler.cb(event, wh->event_handler.ctx);
  }
}

int
ncw_getch(void) {
  return wgetch(stdscr);
}
