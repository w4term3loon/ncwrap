#ifndef NCURSES_WRAPPER_H_HEADER_GUARD
#define NCURSES_WRAPPER_H_HEADER_GUARD

typedef struct input_window_t* input_window_t;
typedef struct scroll_window_t* scroll_window_t;

void ncurses_init();
void ncurses_close();

input_window_t input_window_init(int x,int y, int width, const char* title);
void input_window_close(input_window_t input_window);
int input_window_read(input_window_t input_window, char *buff, size_t buff_siz);

scroll_window_t scroll_window_init(int x, int y, int width, int height, const char* title);
void scroll_window_close(scroll_window_t scroll_window);
void scroll_window_newline(scroll_window_t scroll_window, const char* line);

#endif // NCURSES_WRAPPER_H_HEADER_GUARD