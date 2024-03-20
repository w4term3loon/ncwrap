#include "ncwrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cb(void *ctx) { ++*(int *)ctx; }

void delete_cb(void *ctx) {
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
    menu_window_add_option(menu_window, "delete", delete_cb,
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
