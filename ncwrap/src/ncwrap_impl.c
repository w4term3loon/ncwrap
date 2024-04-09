#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ncurses.h"
#include "ncwrap_impl.h"

#include "ncwrap.h"

// VISUAL--------------------------------------------
//    TODO: indicate nothing happened input window (retval?)
//    TODO: indicate nothing happened menu window (retval?)
//    TODO: enable resize of window
//    TODO: add box border art
// --------------------------------------------------

// BUG-----------------------------------------------
//    TODO: RETURN VALUE CHECKING U MORON!!!
//    TODO: consistent namespacing (functions 'ncw_' ?)
//    TODO: menu options can overlap with side of box
//    TODO: use errors in interface
//    TODO: set visibility on functions
// --------------------------------------------------

// FEATURE-------------------------------------------
//    TODO: introduce thread safety ??
//    TODO: terminal window
//    TODO: debug window
//    TODO: game window
//    TODO: custom window
//    TODO: window editor window
//    TODO: save log from scroll window
//    TODO: login window
//    TODO: header menus for windows
//    TODO: error message box
// --------------------------------------------------

#define ncwrap_handle_error                                                    \
    { (void)fprintf(stderr, "ERROR: ncwrap failed during %s.\n", __func__); }

void
ncwrap_init() {
    ncwrap_handle_error;
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

input_window_t
input_window_init(int x, int y, int width, const char *title) {

    // this really should not be an input_window_t since there is a variable
    // length string stored after the struct but its fine for now
    input_window_t iw =
        (input_window_t)malloc(sizeof(struct input_window) + strlen(title) + 1);
    if (NULL == iw) {
        ncwrap_handle_error;
        return (input_window_t)NULL;
    }

    // store title right after the window struct
    iw->title = (char *)(iw + 1);
    (void)strncpy(iw->title, title, strlen(title) + 1);

    // create the window entity with fix 3 height
    iw->window = newwin(3, width, y, x);
    iw->width = width;

    // print the input window
    box(iw->window, 0, 0);
    mvwprintw(iw->window, 0, 1, " %s ", title);
    wrefresh(iw->window);

    return iw;
}

void
input_window_close(input_window_t iw) {
    // delete literal window with library funtion
    delwin(iw->window);
    // delete my struct and the string after it
    free((void *)iw);
    iw = NULL;
}

void delete(char *buff, size_t buff_siz, int idx) {
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

ncwrap_error
input_window_read(input_window_t iw, char *buff, size_t buff_siz) {
    // display cursor at the input line
    wmove(iw->window, 1, 1);
    curs_set(1);
    wrefresh(iw->window);

    int scope = 0;
    int next, cursor = 0;
    int width = iw->width;
    char display[width];
    int capture = 0;
    for (size_t i = 0; i < buff_siz - 1; ++i) {
        capture = 0;
        next = wgetch(iw->window);

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
            window_content_clear(iw->window, iw->title);
            curs_set(0);
            return NCW_OK;

        case 27: //< escape
            wgetch(iw->window);
            switch (wgetch(iw->window)) {
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
        window_content_clear(iw->window, iw->title);

        // only need to display a portion of the buffer
        for (int j = 0; j < width - 1; ++j) {
            display[j] = buff[scope + j];
            if (j == iw->width - 2) {
                display[j] = '\0';
            };
        }

        mvwprintw(iw->window, 1, 1, "%s", display);

        // left indicator
        if (scope != 0) {
            mvwprintw(iw->window, 1, 0, "<");
        }

        // right indicator
        if (i - scope + 1 >= width - 2) {
            mvwprintw(iw->window, 1, width - 1, ">");
        }

        wmove(iw->window, 1, 1 + cursor);
    }

    window_content_clear(iw->window, iw->title);
    curs_set(0);

    return NCW_OK;
}

scroll_window_t
scroll_window_init(int x, int y, int width, int height, const char *title) {
    // store title right after window struct
    scroll_window_t sw = (scroll_window_t)malloc(sizeof(struct scroll_window) +
                                                 strlen(title) + 1);
    if (NULL == sw) {
        ncwrap_handle_error;
        return (scroll_window_t)NULL;
    }

    sw->title = (char *)(sw + 1);
    (void)strncpy(sw->title, title, strlen(title) + 1);

    // create window
    sw->window = newwin(height, width, y, x);
    sw->width = width;
    sw->height = height;

    box(sw->window, 0, 0);
    mvwprintw(sw->window, 0, 1, " %s ", title);
    wrefresh(sw->window);

    // enable scrolling in this window
    scrollok(sw->window, TRUE);

    return sw;
}

void
scroll_window_close(scroll_window_t sw) {
    delwin(sw->window);
    free((void *)sw);
    sw = NULL;
}

ncwrap_error
scroll_window_add_line(scroll_window_t sw, const char *line) {

    if (NULL == line) {
        return NCW_INVALID_PARAM;
    }

    // displace all lines 1 up (literally)
    scroll(sw->window);

    // clear the place of the added line
    wmove(sw->window, sw->height - 2, 1);
    wclrtoeol(sw->window);

    // print the line in the correct place
    mvwprintw(sw->window, sw->height - 2, 1, "%s", line);

    // reconstruct the widnow box
    box(sw->window, 0, 0);
    mvwprintw(sw->window, 0, 1, " %s ", sw->title);
    wrefresh(sw->window);

    return NCW_OK;
}

menu_window_t
menu_window_init(int x, int y, int width, int height, const char *title) {
    // store title right after the struct
    menu_window_t mw =
        (menu_window_t)malloc(sizeof(struct menu_window) + strlen(title) + 1);
    if (NULL == mw) {
        ncwrap_handle_error;
        return (menu_window_t)NULL;
    }

    mw->title = (char *)(mw + 1);
    (void)strncpy(mw->title, title, strlen(title) + 1);

    // create window
    mw->window = newwin(height, width, y, x);
    mw->width = width;
    mw->height = height;
    mw->options = (option_t *)NULL;
    mw->options_num = 0;
    mw->highlight = 0;

    // display menu
    box(mw->window, 0, 0);
    mvwprintw(mw->window, 0, 1, " %s ", title);
    wrefresh(mw->window);

    return mw;
}

void
menu_window_close(menu_window_t mw) {
    delwin(mw->window);
    for (int i = 0; i < mw->options_num; ++i) {
        free((void *)(mw->options + i)->label);
    }
    free((void *)mw->options);
    free((void *)mw);
    mw = NULL;
}

void
menu_window_update(menu_window_t mw) {
    window_content_clear(mw->window, mw->title);
    for (int i = 0; i < mw->options_num; ++i) {
        if (mw->highlight == i) {
            wattron(mw->window, A_REVERSE);
        }
        mvwprintw(mw->window, i + 1, 1, "%s", (mw->options + i)->label);
        if (mw->highlight == i) {
            wattroff(mw->window, A_REVERSE);
        }
    }
    wrefresh(mw->window);
}

void
menu_window_add_option(menu_window_t mw, const char *label, void (*cb)(void *),
                       void *ctx) {

    for (int i = 0; i < mw->options_num; ++i) {
        if (strcmp(mw->options[i].label, label) == 0) {
            return;
        }
    }

    // alocate place for the new option
    option_t *new_options = (option_t *)realloc(
        mw->options, (mw->options_num + 1) * sizeof(option_t));
    if (NULL == new_options) {
        ncwrap_handle_error;
        return;
    }
    mw->options = new_options;

    // init new option
    option_t *new_option = mw->options + mw->options_num;
    new_option->label = (char *)malloc(strlen(label) + 1);
    if (NULL == new_option->label) {
        ncwrap_handle_error;
        return;
    }

    (void)strncpy(new_option->label, label, strlen(label) + 1);
    new_option->cb = cb;
    new_option->ctx = ctx;

    mw->options_num += 1;
    menu_window_update(mw);
}

void
squash(menu_window_t mw, int option_offset) {
    // free dynamically allocated label
    free((void *)(mw->options + option_offset)->label);
    if (option_offset == mw->options_num - 1) {
        return; //< realloc will take care of the last element
    } else {
        // shift all elements back one place, starting after the offset
        for (int i = option_offset; i < mw->options_num - 1; ++i) {
            (mw->options + i)->label = (mw->options + i + 1)->label;
            (mw->options + i)->cb = (mw->options + i + 1)->cb;
            (mw->options + i)->ctx = (mw->options + i + 1)->ctx;
        }
    }
}

void
menu_window_delete_option(menu_window_t mw, const char *label) {
    for (int i = 0; i < mw->options_num; ++i) {
        if (strcmp((mw->options + i)->label, label) == 0) {
            squash(mw, i);

            void *new_options =
                realloc(mw->options, (mw->options_num - 1) * sizeof(option_t));
            if (NULL == new_options) {
                ncwrap_handle_error;
                return;
            }
            mw->options = new_options;

            --mw->options_num;

            // adjust highlight
            if (i <= mw->highlight) {
                --mw->highlight;
            }

            menu_window_update(mw);
            break;
        }
    }
}

void
menu_window_start(menu_window_t mw) {
    int ch;
    while (true) {
        while (true) {
            if (mw->highlight >= mw->options_num) {
                --mw->highlight;
            } else {
                break;
            }
        }

        menu_window_update(mw);

        ch = wgetch(mw->window);
        switch (ch) {
        case 107: //< k
            if (mw->highlight != 0)
                --mw->highlight;
            break;

        case 106: //< j
            if (mw->highlight != mw->options_num - 1)
                ++mw->highlight;
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
            (mw->options + mw->highlight)
                ->cb((mw->options + mw->highlight)->ctx);
            break;

        // do nothing for arrow characters
        case 27:                //< Esc
            wgetch(mw->window); //< swallow next character
            wgetch(mw->window); //< A,B,C,D arrow
            break;

        case 113: //< q
            // stop highlighting
            mw->highlight = -1;
            menu_window_update(mw);
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
window_content_clear(WINDOW *window, char *title) {
    werase(window);
    box(window, 0, 0);
    mvwprintw(window, 0, 1, " %s ", title);
}
