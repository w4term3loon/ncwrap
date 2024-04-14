#ifndef NCWRAP_IMPL_H_HEADER_GUARD
#define NCWRAP_IMPL_H_HEADER_GUARD

struct input_window {
    char *title;
    WINDOW *window;
    int width;
    output_cb cb;
    void *ctx;
    char *buf;
    size_t bufsz, linesz;
    int display_offs;
    int cursor_offs;
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
} update_t;

typedef ncw_err (*handler_cb)(int event, void *window_ctx);
typedef struct {
    handler_cb cb;
    void *ctx;
} handler_t;

typedef struct window {
    update_t update;
    handler_t handler;
    struct window *next;
} window_t;

#endif // NCWRAP_IMPL_H_HEADER_GUARD
