#include <stdlib.h>
#include <string.h>

#include "ncurses.h"
#include "ncwrap.h"

#include "helper.h"
#include "window.h"
#include "menu.h"

ncw_err
ncw_menu_window_init(menu_window_t *mw, int x, int y, int width, int height, const char *title) {

  ncw_err err = NCW_OK;
  if (NULL == title || 2 > width || 2 > height) {
    err = NCW_INVALID_PARAM;
    goto _end;
  }

  // Menu window handle
  *mw = (menu_window_t)malloc(sizeof(struct menu_window));
  if (NULL == *mw) {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _end;
  }

  // Settings
  (*mw)->width = width;
  (*mw)->height = height;
  (*mw)->options = (option_t *)NULL;
  (*mw)->options_num = 0;
  (*mw)->highlight = -1;
  (*mw)->highlight_buf = 0;

  // Title
  (*mw)->title = (char *)malloc(strlen(title) + 1);
  if (NULL != (*mw)->title) {
    (void)safe_strncpy((*mw)->title, title, strlen(title) + 1);
  } else {
    err = NCW_INSUFFICIENT_MEMORY;
    goto _handle;
  }

  // Ncurses window
  (*mw)->window = newwin(height, width, y, x);
  if (NULL == (*mw)->window) {
    err = NCW_NCURSES_FAIL;
    goto _title;
  }

  // Event functions
  struct update_t update = {.cb = menu_window_update, .ctx = (void *)mw};
  struct event_handler_t handler = {.cb = menu_window_handler, .ctx = (void *)mw};
  (*mw)->wh = window_register(update, handler);
  if (NULL == (*mw)->wh) {
    goto _window;
  }

_end:
  return err;
_window:
  delwin((*mw)->window);
_title:
  free((*mw)->title);
_handle:
  free((void *)*mw);
  *mw = NULL;
  goto _end;
}

ncw_err
ncw_menu_window_close(menu_window_t *mw) {

  ncw_err err = NCW_OK;
  if (NULL == *mw) {
    err = NCW_INVALID_PARAM;
    goto end;
  }

  // NOTE: this may not be the best solution
  // since tis loses the handle for the memory
  if (OK != window_clear((*mw)->window) || OK != delwin((*mw)->window)) {
    err = NCW_NCURSES_FAIL;
    // fallthrough
  }

  window_unregister((*mw)->wh);

  for (int i = 0; i < (*mw)->options_num; ++i) {
    free((void *)((*mw)->options + i)->label);
  }

  free((void *)(*mw)->options);
  free((void *)(*mw)->title);
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
ncw_menu_window_add_option(menu_window_t mw, const char *label, void (*cb)(void *), void *ctx) {

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
  option_t *new_options =
      (option_t *)realloc(mw->options, (mw->options_num + 1) * sizeof(option_t));
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

      void *new_options = realloc(mw->options, (mw->options_num - 1) * sizeof(option_t));
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
