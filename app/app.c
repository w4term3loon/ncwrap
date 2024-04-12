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
add_option(void *ctx) {
    input_window_t iw = NULL;
    input_window_init(&iw, 10, 10, 20, "add");
    char buf[20];
    input_window_read(iw, buf, sizeof buf);
    input_window_close(&iw);

    struct add_option_ctx _ctx = *((struct add_option_ctx *)ctx);
    menu_window_add_option(_ctx.mw, buf, _ctx.cb, _ctx.ctx);
}

void
delete_option(void *ctx) {
    input_window_t iw = NULL;
    input_window_init(&iw, 10, 10, 20, "delete");
    char buf[20];
    input_window_read(iw, buf, sizeof buf);
    input_window_close(&iw);

    menu_window_delete_option((menu_window_t)ctx, buf);
}

int
main() {

    ncwrap_error error;

    error = ncwrap_init();
    if (error != NCW_OK) {
        goto end;
    }

    scroll_window_t sw = NULL;
    error = scroll_window_init(&sw, 30, 2, 50, 15, "scroll");
    if (error != NCW_OK) {
        goto init;
    }

    input_window_t iw = NULL;
    error = input_window_init(&iw, 30, 17, 50, "input");
    if (error != NCW_OK) {
        goto scroll;
    }

    menu_window_t mw = NULL;
    error = menu_window_init(&mw, 10, 2, 20, 18, "menu");
    if (error != NCW_OK) {
        goto input;
    }

    int cnt = 0;
    error = menu_window_add_option(mw, "option1", cb, (void *)&cnt);
    if (error != NCW_OK) {
        goto menu;
    }
    menu_window_add_option(mw, "option2", cb, (void *)&cnt);
    if (error != NCW_OK) {
        goto menu;
    }
    struct add_option_ctx aoc;
    aoc.mw = mw;
    aoc.cb = cb;
    aoc.ctx = (void *)&cnt;
    error = menu_window_add_option(mw, "add", add_option, (void *)&aoc);
    if (error != NCW_OK) {
        goto menu;
    }
    error = menu_window_add_option(mw, "delete", delete_option, (void *)mw);
    if (error != NCW_OK) {
        goto menu;
    }
    error = menu_window_start(mw);
    if (error != NCW_OK) {
        goto menu;
    }

    char buf[60] = "";
    while (strcmp(buf, "exit") != 0) {
        error = input_window_read(iw, buf, sizeof buf);
        if (error != NCW_OK) {
            goto menu;
        }
        error = scroll_window_add_line(sw, buf);
        if (error != NCW_OK) {
            goto menu;
        }
    }

menu:
    menu_window_close(&mw);
input:
    input_window_close(&iw);
scroll:
    scroll_window_close(&sw);
init:
    ncwrap_close();
    printf("menu item selected %d times.\n", cnt);
end:
    return error;
}
