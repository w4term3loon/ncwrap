#ifndef NCWRAP_IMPL_H_HEADER_GUARD
#define NCWRAP_IMPL_H_HEADER_GUARD

struct input_window {
    char *title;
    WINDOW *window;
    int width;
};

struct scroll_window {
    char *title;
    WINDOW *window;
    int width, height;
};

typedef struct {
    char *label;
    void (*cb)(void *);
    void *ctx;
} option_t;

struct menu_window {
    char *title;
    WINDOW *window;
    int width, height;
    option_t *options;
    int options_num;
    int highlight;
};

#endif // NCWRAP_IMPL_H_HEADER_GUARD
