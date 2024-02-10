#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ncwrap.h"

int main()
{
    ncurses_init();
    
    scroll_window_t scroll_window = scroll_window_init(30, 2, 50, 15, "scroll");
    input_window_t input_window = input_window_init(30, 17, 50, "input");
    menu_window_t menu_window = menu_window_init(10, 2, 20, 18, "menu");
    
    char buff[60];
    while (strcmp(buff, "exit") != 0)
    {
        input_window_read(input_window, buff, sizeof buff);
        scroll_window_add_line(scroll_window, buff);
    }
    
    input_window_close(input_window);
    scroll_window_close(scroll_window);
    menu_window_close(menu_window);
    
    ncurses_close();
    return 0;
}
