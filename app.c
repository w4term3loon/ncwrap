#include "ncwrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
cb(void *ctx) {
  ++*(int *)ctx;
}

void
scroll_output(char *buf, size_t bufsz, void *ctx) {
  scroll_window_t sw = (scroll_window_t)ctx;
  ncw_scroll_window_add_line(sw, buf);
}

int
main(void) {

  int cnt = 0;
  ncw_err err = NCW_OK;

  err = ncw_init();
  if (err != NCW_OK) {
    goto end;
  } else {
    ncw_log_file("init success");
  }

  menu_window_t mw = NULL;
  err = ncw_menu_window_init(&mw, 1, 0, 15, 15, "menu");
  if (err != NCW_OK) {
    goto input;
  }

  err = ncw_menu_window_add_option(mw, "option1", cb, (void *)&cnt);
  if (err != NCW_OK) {
    goto menu;
  }

  err = ncw_menu_window_add_option(mw, "option2", cb, (void *)&cnt);
  if (err != NCW_OK) {
    goto menu;
  }

  input_window_t iw = NULL;
  err = ncw_input_window_init(&iw, 16, 12, 30, "input", 0);
  if (err != NCW_OK) {
    goto scroll;
  }

  scroll_window_t sw = NULL;
  err = ncw_scroll_window_init(&sw, 16, 0, 30, 12, "scroll");
  if (err != NCW_OK) {
    goto init;
  }

  // set output for the input window
  ncw_input_window_set_output(iw, scroll_output, (void *)sw);

  // event loop
  int event = 0;
  input_window_t diw = NULL;
  for (;;) {
    ncw_update();
    event = ncw_getch();
    switch (event) {

    case ERR:
      break;

    case CTRL('n'):
      ncw_focus_step();
      break;

    case CTRL('p'):
      ncw_input_window_init(&diw, 2, 2, 20, "added", 1);           //< make window popup
      ncw_input_window_set_output(diw, scroll_output, (void *)sw); //< set output
      break;

    case CTRL('x'):
      goto menu;

    default:
      ncw_event_handler(event);
    }
  }

menu:
  ncw_menu_window_close(&mw);
input:
  ncw_input_window_close(&iw);
scroll:
  ncw_scroll_window_close(&sw);
init:
  ncw_close();
  printf("menu item selected %d times.\n", cnt);
end:
  return err;
}
