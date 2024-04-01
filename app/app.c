#include "ncwrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cb(void *ctx) { ++*(int *)ctx; }

struct add_option_ctx {
    menu_window_t menu_window;
    option_cb cb;
    void *ctx;
};
void add_option(void *ctx) {
    input_window_t input_window = input_window_init(10, 10, 20, "add");
    char buff[20];
    input_window_read(input_window, buff, sizeof buff);
    input_window_close(input_window);

    struct add_option_ctx _ctx = *((struct add_option_ctx *)ctx);
    menu_window_add_option(_ctx.menu_window, buff, _ctx.cb, _ctx.ctx);
}

void delete_option(void *ctx) {
    input_window_t input_window = input_window_init(10, 10, 20, "delete");
    char buff[20];
    input_window_read(input_window, buff, sizeof buff);
    input_window_close(input_window);

    menu_window_delete_option((menu_window_t)ctx, buff);
}

int main() {
    ncwrap_init();

    scroll_window_t scroll_window = scroll_window_init(30, 2, 50, 15, "scroll");
    input_window_t input_window = input_window_init(30, 17, 50, "input");
    menu_window_t menu_window = menu_window_init(10, 2, 20, 18, "menu");

    int cnt = 0;
    menu_window_add_option(menu_window, "option1", cb, (void *)&cnt);
    menu_window_add_option(menu_window, "option2", cb, (void *)&cnt);
    struct add_option_ctx aoc;
    aoc.menu_window = menu_window;
    aoc.cb = cb;
    aoc.ctx = (void *)&cnt;
    menu_window_add_option(menu_window, "add", add_option, (void *)&aoc);
    menu_window_add_option(menu_window, "delete", delete_option,
                           (void *)menu_window);
    menu_window_start(menu_window);

    char buff[60];
    while (strcmp(buff, "exit") != 0) {
        input_window_read(input_window, buff, sizeof buff);
        scroll_window_add_line(scroll_window, buff);
    }

    input_window_close(input_window);
    scroll_window_close(scroll_window);
    menu_window_close(menu_window);

    ncwrap_close();
    printf("menu item selected %d times.\n", cnt);
    return 0;
}
