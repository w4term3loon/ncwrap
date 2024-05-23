// VISUAL--------------------------------------------
//    TODO: indicate nothing happened input window (retval?)
//    TODO: indicate nothing happened menu window (retval?)
//    TODO: enable resize of window
//    TODO: add box border art
// --------------------------------------------------

// BUG-----------------------------------------------
//    TODO: (??) separate different type of widnows into different files
//    TODO: PURGE THE FOCUS SYSTEM
//    TODO: set visibility on functions
//    TODO: https://www.man7.org/linux/man-pages/man3/curs_inopts.3x.html
//    TODO: comment for all magic constants
//    TODO: support for popup windows lifecycle management
//    TODO: only refresh windows that had changed (handler called on)
//    TODO: input window flag for popupness -> if set delete when return
//    TODO: when starting no window indicates focus
//    TODO: if input window output is not set segfault
// -------------------------------------------------

// FEATURE-------------------------------------------
//    TODO: err code interpreter fuction on interface
//    TODO: logging with dlt or syslog or stde
//    TODO: introduce thread safety ??
//    TODO: terminal window
//    TODO: debug window
//    TODO: game window
//    TODO: custom window
//    TODO: window editor window
//    TODO: save log from scroll window
//    TODO: plot window
//    TODO: UI windows that can display telemetry data
//    TODO: login window
//    TODO: header menus for windows
//    TODO: macro keys for windows that call callbacks
//    TODO: err message box
// --------------------------------------------------

#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ncurses.h"
#include "ncwrap.h"

#include "window.h"
#include "helper.h"

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

  if (NULL == get_window_handle()) {
    return;
  }

  // update get_window_handle() last
  for (window_handle_t iter = get_window_handle()->next;; iter = iter->next) {
    if (NULL == iter || NULL == iter->update.cb) {
      return;
    }
    iter->update.cb(iter->update.ctx);
    if (get_window_handle() == iter) {
      break;
    }
  }

  // print to scr
  (void)doupdate();

  return;
}

void
ncw_focus_step(void) {

  if (NULL == get_window_handle()) {
    return;
  }

  // notify the last handler
  get_window_handle()->event_handler.cb(FOCUS_OFF, get_window_handle()->event_handler.ctx);

  // find the next handler
  set_window_handle(get_window_handle()->next);
  while (NULL == get_window_handle()->event_handler.cb) {
    set_window_handle(get_window_handle()->next);
  }

  // notify the next handler
  get_window_handle()->event_handler.cb(FOCUS_ON, get_window_handle()->event_handler.ctx);
}

void
ncw_event_handler(int event) {
  get_window_handle()->event_handler.cb(event, get_window_handle()->event_handler.ctx);
}

int
ncw_getch(void) {
  return wgetch(stdscr);
}

