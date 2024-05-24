#include <stdlib.h>
#include <string.h>

#include "ncurses.h"
#include "ncwrap.h"

#include "helper.h"
#include "window.h"
#include "input.h"

ncw_err
ncw_input_window_init(input_window_t *iw, int x, int y, int width, const char *title,
                      char is_popup) {

  ncw_err err = NCW_OK;
  if (NULL == title || 2 >= width) {
    err = NCW_INVALID_PARAM;
    goto _end;
  }

  // Input window handle
  *iw = (input_window_t)malloc(sizeof(struct input_window));
  if (NULL == *iw) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _end;
  }

  // Settings
  (*iw)->is_popup = is_popup;
  (*iw)->width = width;
  (*iw)->cb = NULL;
  (*iw)->ctx = NULL;
  (*iw)->buf_sz = _BUFSZ;
  (*iw)->line_sz = 0;
  (*iw)->display_offs = 0;
  (*iw)->cursor_offs = 0;
  (*iw)->focus = 0;

  // Title
  (*iw)->title = (char *)malloc(strlen(title) + 1);
  if (NULL != (*iw)->title) {
    (void)safe_strncpy((*iw)->title, title, strlen(title) + 1);
  } else {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _handle;
  }

  // Ncurses window
  (*iw)->window = newwin(3, width, y, x);
  if (NULL == (*iw)->window) {
    err = NCW_NCURSES_FAIL;
    goto _title;
  }

  // Event functions
  struct update_t update = {.cb = input_window_update, .ctx = (void *)iw};
  struct event_handler_t handler = {.cb = input_window_handler, .ctx = (void *)iw};
  (*iw)->wh = window_register(update, handler);
  if (NULL == (*iw)->wh) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _window;
  }

  // Set focus if popup
  if ((*iw)->is_popup) {
    set_window_focus((*iw)->wh);
  }

  // Stored buffer
  (*iw)->buf = (char *)calloc(_BUFSZ, sizeof(char));
  if (NULL == (*iw)->buf) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _event;
  }

#define LINE_TERM 1
  // Displayed chunk
  (*iw)->display = (char *)calloc((size_t)(*iw)->width - 2 + LINE_TERM, sizeof(char));
  if (NULL == (*iw)->display) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _buf;
  }
#undef LINE_TERM

_end:
  return err;
_buf:
  free((void *)(*iw)->buf);
_event:
  window_unregister((*iw)->wh);
_window:
  delwin((*iw)->window);
_title:
  free((*iw)->title);
_handle:
  free((void *)*iw);
  *iw = NULL;
  goto _end;
}

ncw_err
ncw_input_window_close(input_window_t *iw) {

  ncw_err err = NCW_OK;
  if (NULL == *iw) {
    err = NCW_INVALID_PARAM;
    goto _end;
  }

  // NOTE: this may not be the best solution
  // since tis loses the handle for the memory
  if (OK != window_clear((*iw)->window) && OK != delwin((*iw)->window)) {
    err = NCW_NCURSES_FAIL;
    // fallthrough
  }

  window_unregister((*iw)->wh);

  free((void *)(*iw)->buf);
  free((void *)(*iw)->display);
  free((void *)(*iw)->title);
  free((void *)*iw);
  *iw = NULL;

_end:
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

  // clear the display buffer
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
    goto _fail;
  }

  // print the portion that should be visible
  if (OK != mvwprintw(iw->window, 1, 1, "%s", iw->display)) {
    free((void *)iw->display);
    goto _fail;
  }

  // left indicator
  if (iw->display_offs != 0) {
    if (OK != mvwprintw(iw->window, 1, 0, "<")) {
      goto _fail;
    }
  }

  // right indicator
  if (iw->line_sz - iw->display_offs + 1 > iw->width - 2) {
    if (OK != mvwprintw(iw->window, 1, iw->width - 1, ">")) {
      goto _fail;
    }
  }

  if (OK != wnoutrefresh(iw->window)) {
    goto _fail;
  }

_end:
  return err;
_fail:
  err = NCW_NCURSES_FAIL;
  goto _end;
}

ncw_err
input_window_handler(int event, void *window_ctx) {

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
      iw->cb(iw->buf, iw->buf_sz, iw->ctx);
    }

    if (iw->is_popup) {
      ncw_focus_step();
      ncw_input_window_close(&iw);
      goto _end;
    }

    free((void *)iw->buf);
    iw->buf = (char *)calloc(_BUFSZ, sizeof(char));
    if (NULL == iw->buf) {
      err = NCW_INSUFFICIENT_MEMORY;
      goto _end;
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
      goto _end;
    }

    // increase buffer size if needed
    if (iw->line_sz == iw->buf_sz - 1) {
      if (iw->buf_sz < _BUFSZMAX) {

        iw->buf_sz += _BUFSZ;
        iw->buf = realloc(iw->buf, iw->buf_sz);
        if (NULL == iw->buf) {
          err = NCW_INSUFFICIENT_MEMORY;
          goto _end;
        }
      } else {
        goto _end;
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

_end:
  return err;
}
