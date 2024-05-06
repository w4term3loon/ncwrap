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

#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ncurses.h"

#include "ncwrap.h"

#include "ncwrap_helper.h"
#include "ncwrap_impl.h"

static window_handle_t g_wh = NULL;
static window_handle_t g_twh = NULL;

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

window_handle_t
window_register(update_t update, event_handler_t handler) {

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

void
ncw_update(void) {

  if (NULL == g_wh) {
    return;
  }

  // update g_wh last
  for (window_handle_t iter = g_wh->next;; iter = iter->next) {
    if (NULL == iter || NULL == iter->update.cb) {
      return;
    }
    iter->update.cb(iter->update.ctx);
    if (g_wh == iter) {
      break;
    }
  }

  // print to scr
  (void)doupdate();

  return;
}

void
ncw_focus_step(void) {

  if (NULL == g_wh) {
    return;
  }

  // notify the last handler
  g_wh->event_handler.cb(FOCUS_OFF, g_wh->event_handler.ctx);

  // find the next handler
  g_wh = g_wh->next;
  while (NULL == g_wh->event_handler.cb) {
    g_wh = g_wh->next;
  }

  // notify the next handler
  g_wh->event_handler.cb(FOCUS_ON, g_wh->event_handler.ctx);
}

void
ncw_event_handler(int event) {
  g_wh->event_handler.cb(event, g_wh->event_handler.ctx);
}

int
ncw_getch(void) {
  return wgetch(stdscr);
}

ncw_err
input_window_update(void *window_ctx);

ncw_err
input_window_handler(int event, void *window_ctx);

ncw_err
ncw_input_window_init(input_window_t *iw, int x, int y, int width,
                      const char *title, char is_popup) {

  ncw_err err = NCW_OK;

  if (NULL == title) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  if (2 >= width) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  // this really should not be an input_window_t since there is a variable
  // length string stored after the struct but its fine for now
  *iw = (input_window_t)malloc(sizeof(struct input_window) + strlen(title) + 1);
  if (NULL == *iw) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto end;
  }
  // store title right after the window struct
  (*iw)->title = (char *)(*iw + 1);
  (void)safe_strncpy((*iw)->title, title, strlen(title) + 1);

  // create the window entity with fix 3 height
  (*iw)->window = newwin(3, width, y, x);
  if (NULL == (*iw)->window) {
    err = NCW_NCURSES_FAIL;
    goto clean;
  }

  // create structs for the event handler registration
  update_t update = {.cb = input_window_update, .ctx = (void *)iw};
  event_handler_t handler = {.cb = input_window_handler, .ctx = (void *)iw};

  // register window for the event handler
  (*iw)->wh = window_register(update, handler);
  if (NULL == (*iw)->wh) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto delw;
  }

  (*iw)->width = width;

  // set output
  (*iw)->cb = 0;
  (*iw)->ctx = NULL;

  (*iw)->buf = (char *)calloc(_BUFSZ, sizeof(char));
  if (NULL == (*iw)->buf) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto unreg;
  }

  // +1 for terminating '\0'
  (*iw)->display = (char *)calloc((size_t)(*iw)->width - 2 + 1, sizeof(char));
  if (NULL == (*iw)->display) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto cleanall;
  }

  (*iw)->buf_sz = _BUFSZ;
  (*iw)->line_sz = 0;
  (*iw)->display_offs = 0;
  (*iw)->cursor_offs = 0;
  (*iw)->focus = 0;

  // borrow focus for popup
  (*iw)->is_popup = is_popup;
  if (is_popup) {
    g_twh = g_wh;
    g_wh = (*iw)->wh;
    g_twh->event_handler.cb(FOCUS_OFF, g_wh->event_handler.ctx);
    g_wh->event_handler.cb(FOCUS_ON, g_wh->event_handler.ctx);
  }

end:
  return err;
cleanall:
  free((void *)(*iw)->buf);
unreg:
  window_unregister((*iw)->wh);
delw:
  delwin((*iw)->window);
clean:
  free((void *)*iw);
  *iw = NULL;
  goto end;
}

ncw_err
ncw_input_window_close(input_window_t *iw) {

  ncw_err err = NCW_OK;

  if (NULL == *iw) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  if (OK != window_clear((*iw)->window)) {
    err = NCW_NCURSES_FAIL;
    goto end;
  }

  if (OK != delwin((*iw)->window)) {
    err = NCW_NCURSES_FAIL;
    goto end;
  }

  window_unregister((*iw)->wh);

  free((void *)(*iw)->buf);
  free((void *)(*iw)->display);
  free((void *)*iw);
  *iw = NULL;

end:
  return err;
}

ncw_err
ncw_input_window_set_output(input_window_t iw, output_cb cb, void *ctx) {

  if (NULL == cb || NULL == ctx) {
    return NCW_INVALID_PARAM;
  }

  iw->cb = cb;
  iw->ctx = ctx;

  return NCW_OK;
}

ncw_err
input_window_update(void *window_ctx) {

  ncw_err err = NCW_OK;
  input_window_t iw = *((input_window_t *)window_ctx);

  memset(iw->display, 0, (size_t)iw->width - 2 + 1);

  // -1 for the cursor
  for (size_t i = 0; i < iw->width - 2 - 1; ++i) {
    iw->display[i] = iw->buf[iw->display_offs + i];
  }

  // insert cursor
  if (FOCUS_ON == iw->focus) {
    ins(iw->display, iw->width - 2 + 1, iw->cursor_offs, '|');
  }

  if (OK != window_content_clear(iw->window, iw->title)) {
    free((void *)iw->display);
    goto fail;
  }

  // print the portion that should be visible
  if (OK != mvwprintw(iw->window, 1, 1, "%s", iw->display)) {
    free((void *)iw->display);
    goto fail;
  }

  // left indicator
  if (iw->display_offs != 0) {
    if (OK != mvwprintw(iw->window, 1, 0, "<")) {
      goto fail;
    }
  }

  // right indicator
  if (iw->line_sz - iw->display_offs + 1 > iw->width - 2) {
    if (OK != mvwprintw(iw->window, 1, iw->width - 1, ">")) {
      goto fail;
    }
  }

  if (OK != wnoutrefresh(iw->window)) {
    goto fail;
  }

end:
  return err;
fail:
  err = NCW_NCURSES_FAIL;
  goto end;
}

ncw_err
input_window_handler(int event, void *window_ctx) {

  // printf("event:%d->", event);
  ncw_err err = NCW_OK;
  input_window_t iw = *((input_window_t *)window_ctx);

  switch (event) {
  case FOCUS_ON:
    iw->focus = FOCUS_ON;
    break;

  case FOCUS_OFF:
    iw->focus = FOCUS_OFF;
    break;

  case 127:
  case KEY_BACKSPACE:
    // if there are characters before the cursor that can be deleted
    if (iw->display_offs != 0 || iw->cursor_offs != 0) {

      del(iw->buf, iw->buf_sz, iw->display_offs + iw->cursor_offs - 1);

      if (iw->display_offs != 0 &&
          (iw->cursor_offs + iw->display_offs == iw->line_sz ||
           iw->cursor_offs <= iw->width)) {
        iw->display_offs -= 1;
      } else {
        iw->cursor_offs -= 1;
      }

      iw->line_sz -= 1;

    } else { /* indicate nothing happened */
    }
    break;

  case '\n':
  case '\r':
  case KEY_ENTER:

    // return the input string
    if (NULL != iw->cb) {
      printf("%p\n", NULL);
      iw->cb(iw->buf, iw->buf_sz, iw->ctx);
    }

    // close after input
    if (iw->is_popup) {
      if (g_wh == g_wh->next) {
        g_wh = NULL;
        g_twh = NULL;
      } else {
        g_wh->event_handler.cb(FOCUS_OFF, g_wh->event_handler.ctx);
        g_twh->event_handler.cb(FOCUS_ON, g_wh->event_handler.ctx);
        g_wh = g_twh;
        g_twh = NULL;
      }
      ncw_input_window_close(&iw);
      goto end;
    }

    free((void *)iw->buf);
    iw->buf = (char *)calloc(_BUFSZ, sizeof(char));
    if (NULL == iw->buf) {
      err = NCW_INSUFFICIENT_MEMORY;
      goto end;
    }

    iw->buf_sz = _BUFSZ;

    iw->line_sz = 0;
    iw->display_offs = 0;
    iw->cursor_offs = 0;

    break;

  case KEY_LEFT:
    if (iw->cursor_offs != 0) {
      iw->cursor_offs -= 1;
    } else {
      if (iw->display_offs != 0) {
        iw->display_offs -= 1;
      }
    }
    break;

  case KEY_RIGHT:
    if (iw->cursor_offs != iw->width - 3) {
      if (iw->cursor_offs < iw->line_sz) {
        iw->cursor_offs += 1;
      }
    } else if (iw->cursor_offs + iw->display_offs != iw->line_sz) {
      iw->display_offs += 1;
    }
    break;

  case KEY_UP:
  case KEY_DOWN:
    break;

  default: //< printable character

    // ignore control ascii
    if (event < 32 || event > 127) {
      goto end;
    }

    // increase buffer size if needed
    if (iw->line_sz == iw->buf_sz - 1) {
      if (iw->buf_sz < _BUFSZMAX) {

        iw->buf_sz += _BUFSZ;
        iw->buf = realloc(iw->buf, iw->buf_sz);
        if (NULL == iw->buf) {
          err = NCW_INSUFFICIENT_MEMORY;
          goto end;
        }
      } else {
        goto end;
      }
    }

    ins(iw->buf, iw->buf_sz, iw->display_offs + iw->cursor_offs, (char)event);

    ++iw->line_sz;

    if (iw->width - 3 == iw->cursor_offs) {
      ++iw->display_offs;
    } else {
      ++iw->cursor_offs;
    }
  }

end:
  return err;
}

ncw_err
scroll_window_update(void *window_ctx);

ncw_err
ncw_scroll_window_init(scroll_window_t *sw, int x, int y, int width, int height,
                       const char *title) {

  ncw_err err = NCW_OK;

  if (NULL == title) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  // store title right after window struct
  *sw =
      (scroll_window_t)malloc(sizeof(struct scroll_window) + strlen(title) + 1);
  if (NULL == *sw) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto end;
  }

  (*sw)->title = (char *)(*sw + 1);
  (void)safe_strncpy((*sw)->title, title, strlen(title) + 1);

  // create window
  (*sw)->window = newwin(height, width, y, x);
  if (NULL == (*sw)->window) {
    err = NCW_NCURSES_FAIL;
    goto clean;
  }

  update_t update = {.cb = scroll_window_update, .ctx = (void *)sw};
  event_handler_t handler = {.cb = NULL, .ctx = NULL};

  // register window for the event handler
  (*sw)->wh = window_register(update, handler);
  if (NULL == (*sw)->wh) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto cleanall;
  }

  (*sw)->width = width;
  (*sw)->height = height;
  (*sw)->next_line = NULL;

  // enable scrolling in this window
  scrollok((*sw)->window, TRUE);

end:
  return err;
cleanall:
  delwin((*sw)->window);
clean:
  free((void *)*sw);
  *sw = NULL;
  goto end;
}

ncw_err
ncw_scroll_window_close(scroll_window_t *sw) {

  ncw_err err = NCW_OK;

  if (NULL == *sw) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  if (OK != window_clear((*sw)->window)) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  if (OK != delwin((*sw)->window)) {
    err = NCW_NCURSES_FAIL;
    goto end;
  }

  window_unregister((*sw)->wh);

  free((void *)(*sw)->next_line);
  free((void *)*sw);
  *sw = NULL;

end:
  return err;
}

ncw_err
scroll_window_update(void *window_ctx) {

  ncw_err err = NCW_OK;
  scroll_window_t sw = *((scroll_window_t *)window_ctx);

  // if there is no new next line, skip
  if (NULL == sw->next_line) {
    goto skip;
  }

  // displace all lines 1 up (literally)
  if (OK != scroll(sw->window)) {
    goto fail;
  }

  // clear the place of the added line
  if (OK != wmove(sw->window, sw->height - 2, 1)) {
    goto fail;
  }

  if (OK != wclrtoeol(sw->window)) {
    goto fail;
  }

  // print the line in the correct place
  if (OK != mvwprintw(sw->window, sw->height - 2, 1, "%s", sw->next_line)) {
    goto fail;
  }

  // free next line for new one
  free((void *)sw->next_line);
  sw->next_line = NULL;

skip:
  // reconstruct the widnow box
  if (OK != window_draw_box(sw->window, sw->title)) {
    goto fail;
  }
end:
  return err;
fail:
  err = NCW_NCURSES_FAIL;
  goto end;
}

ncw_err
ncw_scroll_window_add_line(scroll_window_t sw, const char *line) {

  if (NULL != sw->next_line) {
    free(sw->next_line);
  }

  sw->next_line = (char *)malloc(strlen(line) + 1);
  if (NULL == sw->next_line) {
    return NCW_INSUFFICIENT_MEMORY;
  }
  (void)safe_strncpy(sw->next_line, line, strlen(line) + 1);

  return NCW_OK;
}

ncw_err
menu_window_update(void *window_ctx);

ncw_err
menu_window_handler(int event, void *window_ctx);

ncw_err
ncw_menu_window_init(menu_window_t *mw, int x, int y, int width, int height,
                     const char *title) {

  ncw_err err = NCW_OK;

  if (NULL == title) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  // store title right after the struct
  *mw = (menu_window_t)malloc(sizeof(struct menu_window) + strlen(title) + 1);
  if (NULL == *mw) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto end;
  }

  (*mw)->title = (char *)(*mw + 1);
  (void)safe_strncpy((*mw)->title, title, strlen(title) + 1);

  // create window
  (*mw)->window = newwin(height, width, y, x);
  if (NULL == (*mw)->window) {
    err = NCW_NCURSES_FAIL;
    goto clean;
  }

  update_t update = {.cb = menu_window_update, .ctx = (void *)mw};
  event_handler_t handler = {.cb = menu_window_handler, .ctx = (void *)mw};

  // register window for the event handler
  (*mw)->wh = window_register(update, handler);
  if (NULL == (*mw)->wh) {
    goto cleanall;
  }

  (*mw)->width = width;
  (*mw)->height = height;
  (*mw)->options = (option_t *)NULL;
  (*mw)->options_num = 0;
  (*mw)->highlight = -1;
  (*mw)->highlight_buf = 0;

end:
  return err;
cleanall:
  delwin((*mw)->window);
clean:
  free((void *)*mw);
  *mw = NULL;
  goto end;
}

ncw_err
ncw_menu_window_close(menu_window_t *mw) {

  ncw_err err = NCW_OK;

  if (NULL == *mw) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  // if (OK != window_clear((*mw)->window)) {
  //     err = NCW_NCURSES_FAIL;
  //     goto end;
  // }

  if (OK != delwin((*mw)->window)) {
    err = NCW_NCURSES_FAIL;
    goto end;
  }

  window_unregister((*mw)->wh);

  for (int i = 0; i < (*mw)->options_num; ++i) {
    free((void *)((*mw)->options + i)->label);
  }

  free((void *)(*mw)->options);
  free((void *)*mw);
  *mw = NULL;

end:
  return err;
}

ncw_err
menu_window_update(void *window_ctx) {

  ncw_err err = NCW_OK;
  menu_window_t mw = *((menu_window_t *)window_ctx);

  // adjust highlight
  while (true) {
    if (mw->highlight >= mw->options_num) {
      --mw->highlight;
    } else {
      break;
    }
  }

  if (OK != window_content_clear(mw->window, mw->title)) {
    goto fail;
  }

  for (int i = 0; i < mw->options_num; ++i) {

    if (mw->highlight == i) {
      if (OK != wattron(mw->window, A_REVERSE)) {
        goto fail;
      }
    }

    if (OK != mvwprintw(mw->window, i + 1, 1, "%s", (mw->options + i)->label)) {
      goto fail;
    }

    if (mw->highlight == i) {
      if (OK != wattroff(mw->window, A_REVERSE)) {
        goto fail;
      }
    }
  }

  if (OK != wnoutrefresh(mw->window)) {
    goto fail;
  }

end:
  return err;
fail:
  err = NCW_NCURSES_FAIL;
  goto end;
}

ncw_err
ncw_menu_window_add_option(menu_window_t mw, const char *label,
                           void (*cb)(void *), void *ctx) {

  ncw_err err = NCW_OK;

  if (strlen(label) > mw->width - 2) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  for (int i = 0; i < mw->options_num; ++i) {
    if (strcmp(mw->options[i].label, label) == 0) {
      err = NCW_INVALID_PARAM;
      goto end;
    }
  }

  // alocate place for the new option
  option_t *new_options = (option_t *)realloc(
      mw->options, (mw->options_num + 1) * sizeof(option_t));
  if (NULL == new_options) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto end;
  }

  mw->options = new_options;

  // init new option
  option_t *new_option = mw->options + mw->options_num;
  new_option->label = (char *)malloc(strlen(label) + 1);
  if (NULL == new_option->label) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto end;
  }

  (void)safe_strncpy(new_option->label, label, strlen(label) + 1);
  new_option->cb = cb;
  new_option->ctx = ctx;

  mw->options_num += 1;

end:
  return err;
}

ncw_err
ncw_menu_window_delete_option(menu_window_t mw, const char *label) {

  ncw_err err = NCW_OK;

  if (NULL == label) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  for (int i = 0; i < mw->options_num; ++i) {
    if (strcmp((mw->options + i)->label, label) == 0) {

      squash(mw, i);

      void *new_options =
          realloc(mw->options, (mw->options_num - 1) * sizeof(option_t));
      if (NULL == new_options) {
        err = NCW_INSUFFICIENT_MEMORY;
        goto end;
      }

      mw->options = new_options;
      --mw->options_num;

      // adjust highlight
      if (i <= mw->highlight) {
        --mw->highlight;
      }

      goto end;
    }
  }

end:
  return err;
}

ncw_err
menu_window_handler(int event, void *window_ctx) {

  ncw_err err = NCW_OK;
  menu_window_t mw = *((menu_window_t *)window_ctx);

  switch (event) {
  case FOCUS_ON:
    mw->highlight = mw->highlight_buf;
    break;

  case 107: //< k
    if (mw->highlight != 0)
      --mw->highlight;
    break;

  case 106: //< j
    if (mw->highlight != mw->options_num - 1)
      ++mw->highlight;
    break;

  case '\n':
  case '\r':
  case KEY_ENTER:
    (mw->options + mw->highlight)->cb((mw->options + mw->highlight)->ctx);
    break;

  // do nothing for arrow characters
  case KEY_DOWN:
  case KEY_UP:
  case KEY_LEFT:
  case KEY_RIGHT:
    break;

  // stop highlighting
  case FOCUS_OFF:
    mw->highlight_buf = mw->highlight;
    mw->highlight = -1;
    break;

  default:;
  }

  return err;
}
