#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ncwrap.h"

int main()
{
    ncurses_init();
    
    scroll_window_t scroll_window = scroll_window_init(10, 2, 50, 15, "scroll");
    input_window_t input_window = input_window_init(10, 17, 50, "input");
    
    char buff[60];
    while (strcmp(buff, "exit") != 0)
    {
        input_window_read(input_window, buff, sizeof buff);
        scroll_window_newline(scroll_window, buff);
    }
    
    input_window_close(input_window);
    scroll_window_close(scroll_window);
    
    ncurses_close();
    return 0;
}
