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
    input_window_t iw = input_window_init(10, 10, 20, "add");
    char buff[20];
    input_window_read(iw, buff, sizeof buff);
    input_window_close(iw);

    struct add_option_ctx _ctx = *((struct add_option_ctx *)ctx);
    menu_window_add_option(_ctx.mw, buff, _ctx.cb, _ctx.ctx);
}

void
delete_option(void *ctx) {
    input_window_t iw = input_window_init(10, 10, 20, "delete");
    char buff[20];
    input_window_read(iw, buff, sizeof buff);
    input_window_close(iw);

    menu_window_delete_option((menu_window_t)ctx, buff);
}

int
main() {
    ncwrap_init();

    scroll_window_t sw = scroll_window_init(30, 2, 50, 15, "scroll");
    input_window_t iw = input_window_init(30, 17, 50, "input");
    menu_window_t mw = menu_window_init(10, 2, 20, 18, "menu");

    int cnt = 0;
    menu_window_add_option(mw, "option1", cb, (void *)&cnt);
    menu_window_add_option(mw, "option2", cb, (void *)&cnt);
    struct add_option_ctx aoc;
    aoc.mw = mw;
    aoc.cb = cb;
    aoc.ctx = (void *)&cnt;
    menu_window_add_option(mw, "add", add_option, (void *)&aoc);
    menu_window_add_option(mw, "delete", delete_option, (void *)mw);
    menu_window_start(mw);

    char buff[60] = "";
    while (strcmp(buff, "exit") != 0) {
        input_window_read(iw, buff, sizeof buff);
        scroll_window_add_line(sw, buff);
    }

    input_window_close(iw);
    scroll_window_close(sw);
    menu_window_close(mw);

    ncwrap_close();
    printf("menu item selected %d times.\n", cnt);
    return 0;
}
