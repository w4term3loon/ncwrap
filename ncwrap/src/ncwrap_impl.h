#ifndef NCWRAP_IMPL_H_HEADER_GUARD
#define NCWRAP_IMPL_H_HEADER_GUARD

typedef struct {
    char *title;
    WINDOW *window;
    int width;
} input_window_t;

typedef struct {
    char *title;
    WINDOW *window;
    int width, height;
} scroll_window_t;

typedef struct {
    char *label;
    void (*cb)(void *);
    void *ctx;
} option_t;

typedef struct {
    char *title;
    WINDOW *window;
    int width, height;
    option_t *options;
    int options_num;
} menu_window_t;

void
clear_window_content(WINDOW *window, char *title);

#endif // NCWRAP_IMPL_H_HEADER_GUARD
