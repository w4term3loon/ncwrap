#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ncurses.h"
#include "ncwrap_impl.h"

// VISUAL--------------------------------------------
//    TODO: indicate nothing happened
//    TODO: enable resize of window
//    TODO: add box border art
// --------------------------------------------------

// BUG-----------------------------------------------
//    TODO: unrecognised characters (TAB)
//    TODO: handle multiple menu items with the same name (delete)
//    TODO: deleting an option from menu should make the highlight stay on the
//    same option (make highligt an attribute of menu struct)
//    TODO: RETURN VALUE CHECKING U MORON!!!
// --------------------------------------------------

// FEATURE-------------------------------------------
//    TODO: introduce thread safety ??
//    TODO: terminal window
//    TODO: debug window
// --------------------------------------------------

void
menu_window_update(menu_window_t *menu_window, int highlight);

void
handle_error(const char *ctx) {
    (void)fprintf(stderr, "ERROR: ncwrap failed during %s.\n", ctx);
}

#define ncwrap_error handle_error(__func__)

void
ncwrap_init() {
    (void)initscr();
    (void)nonl();
    (void)cbreak();
    (void)noecho();
    (void)curs_set(0);
}

void
ncwrap_close() {
    (void)endwin();
}

input_window_t *
input_window_init(int x, int y, int width, const char *title) {

    // this really should not be an input_window_t since there is a variable
    // length string stored after the struct but its fine for now
    input_window_t *input_window =
        (input_window_t *)malloc(sizeof(input_window_t) + strlen(title) + 1);
    if (input_window == NULL) {
        ncwrap_error;
        return (input_window_t *)NULL;
    }

    // store title right after the window struct
    input_window->title = (char *)(input_window + 1);
    (void)strncpy(input_window->title, title, strlen(title) + 1);

    // create the window entity with fix 3 height
    input_window->window = newwin(3, width, y, x);
    input_window->width = width;

    // print the input window
    box(input_window->window, 0, 0);
    mvwprintw(input_window->window, 0, 1, " %s ", title);
    wrefresh(input_window->window);

    return input_window;
}

void
input_window_close(input_window_t *input_window) {
    // delete literal window with library funtion
    delwin(input_window->window);
    // delete my struct and the string after it
    free((void *)input_window);
    input_window = NULL;
}

void delete (char *buff, size_t buff_siz, int idx) {
    for (size_t i = 0; i < buff_siz; ++i) {
        if (i >= idx) {
            buff[i] = buff[i + 1];
        }
    }
}

void
insert(char *buff, size_t buff_siz, int idx, char ch) {
    for (size_t i = buff_siz - 1; i > idx; --i) {
        if (i > idx) {
            buff[i] = buff[i - 1];
        }
    }

    buff[idx] = ch;
}

int
input_window_read(input_window_t *input_window, char *buff, size_t buff_siz) {
    // display cursor at the input line
    wmove(input_window->window, 1, 1);
    curs_set(1);
    wrefresh(input_window->window);

    int scope = 0;
    int next, cursor = 0;
    int width = input_window->width;
    char display[width];
    int capture = 0;
    for (size_t i = 0; i < buff_siz - 1; ++i) {
        capture = 0;
        next = wgetch(input_window->window);

        // disgusting but did not want one more indentation
        if (31 < next && next < 128) {
            ++capture;
        }

        switch (next) {
        case 127: //< backspace
        case KEY_DC:
        case KEY_BACKSPACE:
            if (scope != 0 || cursor != 0) {
                delete (buff, buff_siz, scope + cursor - 1);
                if (scope != 0 && (cursor + scope == i || cursor <= width)) {
                    scope -= 1;
                } else {
                    cursor -= 1;
                }
                i -= 2;
            } else {
                --i; //< indicate nothing happened
            }
            break;

        case '\n': //< return
        case '\r':
        case KEY_ENTER:
            buff[i] = '\0';
            clear_window_content(input_window->window, input_window->title);
            curs_set(0);
            return i + 1;
            break;

        case 27: //< escape
            wgetch(input_window->window);
            switch (wgetch(input_window->window)) {
            case 'D': //< left arrow
            case KEY_LEFT:
                if (cursor != 0) {
                    cursor -= 1;
                } else {
                    if (scope != 0) {
                        scope -= 1;
                    }
                }
                break;

            case 'C': //< right arrow
            case KEY_RIGHT:
                if (cursor != width - 3) {
                    if (cursor < i) {
                        cursor += 1;
                    }
                } else if (cursor + scope != i) {
                    scope += 1;
                }
                break;

            case 'A': //< up arrow
            case 'B': //< down arrow
            case KEY_UP:
            case KEY_DOWN:
                break;

            default:;
            }
            --i;
            break;

        default:
            if (capture) {
                insert(buff, buff_siz, scope + cursor, (char)next);
                if (width - 3 == cursor) {
                    scope += 1;
                } else {
                    cursor += 1;
                }

            } else
                break;
        }

        buff[i + 1] = '\0';
        clear_window_content(input_window->window, input_window->title);

        // only need to display a portion of the buffer
        for (int j = 0; j < width - 1; ++j) {
            display[j] = buff[scope + j];
            if (j == input_window->width - 2) {
                display[j] = '\0';
            };
        }

        mvwprintw(input_window->window, 1, 1, "%s", display);

        // left indicator
        if (scope != 0) {
            mvwprintw(input_window->window, 1, 0, "<");
        }

        // right indicator
        if (i - scope + 1 >= width - 2) {
            mvwprintw(input_window->window, 1, width - 1, ">");
        }

        wmove(input_window->window, 1, 1 + cursor);
    }

    clear_window_content(input_window->window, input_window->title);
    curs_set(0);

    return buff_siz;
}

scroll_window_t *
scroll_window_init(int x, int y, int width, int height, const char *title) {
    // store title right after window struct
    scroll_window_t *scroll_window =
        (scroll_window_t *)malloc(sizeof(scroll_window_t) + strlen(title) + 1);
    if (scroll_window == NULL) {
        ncwrap_error;
        return (scroll_window_t *)NULL;
    }

    scroll_window->title = (char *)(scroll_window + 1);
    (void)strncpy(scroll_window->title, title, strlen(title) + 1);

    // create window
    scroll_window->window = newwin(height, width, y, x);
    scroll_window->width = width;
    scroll_window->height = height;

    box(scroll_window->window, 0, 0);
    mvwprintw(scroll_window->window, 0, 1, " %s ", title);
    wrefresh(scroll_window->window);

    // enable scrolling in this window
    scrollok(scroll_window->window, TRUE);

    return scroll_window;
}

void
scroll_window_close(scroll_window_t *scroll_window) {
    delwin(scroll_window->window);
    free((void *)scroll_window);
    scroll_window = NULL;
}

void
scroll_window_add_line(scroll_window_t *scroll_window, const char *line) {
    // displace all lines 1 up (literally)
    scroll(scroll_window->window);

    // clear the place of the added line
    wmove(scroll_window->window, scroll_window->height - 2, 1);
    wclrtoeol(scroll_window->window);

    // print the line in the correct place
    mvwprintw(scroll_window->window, scroll_window->height - 2, 1, "%s", line);

    // reconstruct the widnow box
    box(scroll_window->window, 0, 0);
    mvwprintw(scroll_window->window, 0, 1, " %s ", scroll_window->title);
    wrefresh(scroll_window->window);
}

menu_window_t *
menu_window_init(int x, int y, int width, int height, const char *title) {
    // store title right after the struct
    menu_window_t *menu_window =
        (menu_window_t *)malloc(sizeof(menu_window_t) + strlen(title) + 1);
    if (menu_window == NULL) {
        ncwrap_error;
        return (menu_window_t *)NULL;
    }

    menu_window->title = (char *)(menu_window + 1);
    (void)strncpy(menu_window->title, title, strlen(title) + 1);

    // create window
    menu_window->window = newwin(height, width, y, x);
    menu_window->width = width;
    menu_window->height = height;
    menu_window->options = (option_t *)NULL;
    menu_window->options_num = 0;

    // display menu
    box(menu_window->window, 0, 0);
    mvwprintw(menu_window->window, 0, 1, " %s ", title);
    wrefresh(menu_window->window);

    return menu_window;
}

void
menu_window_close(menu_window_t *menu_window) {
    delwin(menu_window->window);
    for (int i = 0; i < menu_window->options_num; ++i) {
        free((void *)(menu_window->options + i)->label);
    }
    free((void *)menu_window->options);
    free((void *)menu_window);
    menu_window = NULL;
}

void
menu_window_add_option(menu_window_t *menu_window, const char *label,
                       void (*cb)(void *), void *ctx) {
    // alocate place for the new option
    option_t *new_options =
        (option_t *)realloc(menu_window->options,
                            (menu_window->options_num + 1) * sizeof(option_t));
    if (new_options == NULL) {
        ncwrap_error;
        return;
    }
    menu_window->options = new_options;

    // init new option
    option_t *new_option = menu_window->options + menu_window->options_num;
    new_option->label = (char *)malloc(strlen(label) + 1);
    if (new_option->label == NULL) {
        ncwrap_error;
        return;
    }

    (void)strncpy(new_option->label, label, strlen(label) + 1);
    new_option->cb = cb;
    new_option->ctx = ctx;

    menu_window->options_num += 1;
    menu_window_update(menu_window, 0);
}

void
squash(menu_window_t *menu_window, int option_offset) {
    // free dynamically allocated label
    free((void *)(menu_window->options + option_offset)->label);
    if (option_offset == menu_window->options_num - 1) {
        return; //< realloc will take care of the last element
    } else {
        // shift all elements back one place, starting after the offset
        for (int i = option_offset; i < menu_window->options_num - 1; ++i) {
            (menu_window->options + i)->label =
                (menu_window->options + i + 1)->label;
            (menu_window->options + i)->cb = (menu_window->options + i + 1)->cb;
            (menu_window->options + i)->ctx =
                (menu_window->options + i + 1)->ctx;
        }
    }
}

void
menu_window_delete_option(menu_window_t *menu_window, const char *label) {
    for (int i = 0; i < menu_window->options_num; ++i) {
        if (strcmp((menu_window->options + i)->label, label) == 0) {
            squash(menu_window, i);

            void *new_options =
                realloc(menu_window->options,
                        (menu_window->options_num - 1) * sizeof(option_t));
            if (new_options == NULL) {
                ncwrap_error;
                return;
            }
            menu_window->options = new_options;

            --menu_window->options_num;
            menu_window_update(menu_window, 0);
            break;
        }
    }
}

void
menu_window_update(menu_window_t *menu_window, int highlight) {

    clear_window_content(menu_window->window, menu_window->title);
    for (int i = 0; i < menu_window->options_num; ++i) {
        if (highlight == i) {
            wattron(menu_window->window, A_REVERSE);
        }
        mvwprintw(menu_window->window, i + 1, 1, "%s",
                  (menu_window->options + i)->label);
        if (highlight == i) {
            wattroff(menu_window->window, A_REVERSE);
        }
    }

    wrefresh(menu_window->window);
}

void
menu_window_start(menu_window_t *menu_window) {
    int ch;
    int highlight = 0;
    while (true) {
        while (true) {
            if (highlight >= menu_window->options_num) {
                --highlight;
            } else {
                break;
            }
        }

        menu_window_update(menu_window, highlight);

        ch = wgetch(menu_window->window);
        switch (ch) {
        case 107: // k
            if (highlight != 0)
                --highlight;
            break;

        case 106: // j
            if (highlight != menu_window->options_num - 1)
                ++highlight;
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
            (menu_window->options + highlight)
                ->cb((menu_window->options + highlight)->ctx);
            break;

        case 27:  //< Esc
        case 113: //< q
            menu_window_update(menu_window, -1);
            goto exit;

        default:;
        }

        if (false) {
        exit:
            break;
        }
    }
}

void
clear_window_content(WINDOW *window, char *title) {
    werase(window);
    box(window, 0, 0);
    mvwprintw(window, 0, 1, " %s ", title);
}
