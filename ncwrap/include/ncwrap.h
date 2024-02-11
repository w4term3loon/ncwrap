#ifndef NCWRAP_H_HEADER_GUARD
#define NCWRAP_H_HEADER_GUARD

typedef struct input_window_t* input_window_t;
typedef struct scroll_window_t* scroll_window_t;
typedef struct menu_window_t* menu_window_t;

void ncurses_init();
void ncurses_close();

input_window_t input_window_init(int x, int y, int width, const char* title);
void input_window_close(input_window_t input_window);
int input_window_read(input_window_t input_window, char *buff, size_t buff_sz);

scroll_window_t scroll_window_init(int x, int y, int width, int height, const char* title);
void scroll_window_close(scroll_window_t scroll_window);
void scroll_window_add_line(scroll_window_t scroll_window, const char* line);

typedef void (*option_cb)(void *ctx);
menu_window_t menu_window_init(int x, int y, int width, int height, const char* title);
void menu_window_close(menu_window_t menu_window);
void menu_window_start(menu_window_t menu_window);
void menu_window_add_option(menu_window_t menu_window, const char* title, option_cb cb, void *ctx);

#endif // NCWRAP_H_HEADER_GUARD
