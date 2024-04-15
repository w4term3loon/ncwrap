#include "ncwrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
cb(void *ctx) {
    ++*(int *)ctx;
}

struct add_option_ctx {
    menu_window_t mw;
    option_cb cb;
    void *ctx;
};

void
asd_cb(char *buf, size_t bufsz, void *ctx) {
    char *ctxp = (char *)ctx;
    (void)strncpy(ctxp, buf, bufsz);
}

void
add_option(void *ctx) {

    input_window_t iw = NULL;
    ncw_input_window_init(&iw, 10, 10, 20, "add");

    char name[50];
    ncw_input_window_set_output(iw, asd_cb, (void *)name);
    ncw_input_window_close(&iw);

    struct add_option_ctx _ctx = *((struct add_option_ctx *)ctx);
    ncw_menu_window_add_option(_ctx.mw, name, _ctx.cb, _ctx.ctx);
}

// void
// delete_option(void *ctx) {
//
//     input_window_t iw = NULL;
//     ncw_input_window_init(&iw, 10, 10, 20, "delete");
//
//     char buf[20];
//     ncw_input_window_read(iw, buf, sizeof buf);
//     ncw_input_window_close(&iw);
//
//     ncw_menu_window_delete_option((menu_window_t)ctx, buf);
// }

void
scroll_output(char *buf, size_t bufsz, void *ctx) {
    scroll_window_t sw = (scroll_window_t)ctx;
    ncw_scroll_window_add_line(sw, buf);
}

int
main(void) {

    ncw_err err = NCW_OK;

    err = ncw_init();
    if (err != NCW_OK) {
        goto end;
    }

    input_window_t iw = NULL;
    err = ncw_input_window_init(&iw, 30, 17, 50, "input");
    if (err != NCW_OK) {
        goto scroll;
    }

    scroll_window_t sw = NULL;
    err = ncw_scroll_window_init(&sw, 30, 2, 50, 15, "scroll");
    if (err != NCW_OK) {
        goto init;
    }

    // set output for the input window
    ncw_input_window_set_output(iw, scroll_output, (void *)sw);

    menu_window_t mw = NULL;
    err = ncw_menu_window_init(&mw, 10, 2, 20, 18, "menu");
    if (err != NCW_OK) {
        goto input;
    }

    int cnt = 0;
    err = ncw_menu_window_add_option(mw, "option1", cb, (void *)&cnt);
    if (err != NCW_OK) {
        goto menu;
    }

    err = ncw_menu_window_add_option(mw, "option2", cb, (void *)&cnt);
    if (err != NCW_OK) {
        goto menu;
    }

    struct add_option_ctx aoc;
    aoc.mw = mw;
    aoc.cb = cb;
    aoc.ctx = (void *)&cnt;

    err = ncw_menu_window_add_option(mw, "add", add_option, (void *)&aoc);
    if (err != NCW_OK) {
        goto menu;
    }

    err = ncw_start();
    if (err != NCW_OK) {
        goto input;
    }

    // err = ncw_menu_window_add_option(mw, "delete", delete_option, (void
    // *)mw); if (err != NCW_OK) {
    //     goto menu;
    // }

    // do {

    //     err = ncw_input_window_read(iw, buf, sizeof buf);
    //     if (err != NCW_OK) {
    //         goto menu;
    //     }

    //     err = ncw_scroll_window_add_line(sw, buf);
    //     if (err != NCW_OK) {
    //         goto menu;
    //     }

    // } while (strcmp(buf, "exit") != 0);

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
