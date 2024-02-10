#ifndef NCURSES_WRAPPER_IMPL_H_HEADER_GUARD
#define NCURSES_WRAPPER_IMPL_H_HEADER_GUARD

typedef struct {
    char* title;
    WINDOW* window;
    int width;
} input_window_t;

typedef struct {
    char* title;
    WINDOW* window;
    int width, height;
} scroll_window_t;

typedef struct {
    char* title;
    WINDOW* window;
    int width, height;
} menu_window_t;

typedef void (*option_cb)(void *ctx);

void clear_window_content(WINDOW* window, char* title);

#endif // NCURSES_WRAPPER_IMPL_H_HEADER_GUARD
