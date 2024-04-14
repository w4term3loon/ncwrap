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
    char *next_line;
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

typedef ncw_err (*update_cb)(void *window_ctx);
typedef struct {
    update_cb cb;
    void *ctx;
} update;

typedef ncw_err (*handler_cb)(int event, void *window_ctx);
typedef struct {
    handler_cb cb;
    void *ctx;
} handler;

#endif // NCWRAP_IMPL_H_HEADER_GUARD
