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
//    TODO: safe strncpy with guaranteed \0 at the end
//    TODO: separate: erase window, draw box
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

ncwrap_error
ncwrap_init() {

    (void)initscr(); /* init ncurses library */
    (void)nonl();    /* tell ncurses not to do NL->CR/NL on output */
    (void)cbreak();  /* take input chars one at a time, no wait for \n */
    (void)echo();    /* echo input - in color */

    (void)curs_set(0); /* set cursor to invisible  */

    return NCW_OK;
}

ncwrap_error
ncwrap_close() {

    (void)endwin(); /* terminate ncurses library */

    return NCW_OK;
}

ncwrap_error
input_window_init(input_window_t *iw, int x, int y, int width,
                  const char *title) {

    ncwrap_error error = NCW_OK;

    // this really should not be an input_window_t since there is a variable
    // length string stored after the struct but its fine for now
    *iw =
        (input_window_t)malloc(sizeof(struct input_window) + strlen(title) + 1);
    if (NULL == *iw) {
        error = NCW_INSUFFICIENT_MEMORY;
        goto end;
    }

    // store title right after the window struct
    (*iw)->title = (char *)(*iw + 1);
    (void)strncpy((*iw)->title, title, strlen(title) + 1);

    // create the window entity with fix 3 height
    (*iw)->window = newwin(3, width, y, x);
    if (NULL == (*iw)->window) {
        error = NCW_NCURSES_FAIL;
        goto clean;
    }

    (*iw)->width = width;

    if (OK != box((*iw)->window, 0, 0)) {
        error = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    if (OK != mvwprintw((*iw)->window, 0, 1, " %s ", title)) {
        error = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    if (OK != wrefresh((*iw)->window)) {
        error = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    goto end;

cleanall:
    delwin((*iw)->window);
clean:
    free((void *)*iw);
    *iw = NULL;
end:
    return error;
}

ncwrap_error
input_window_close(input_window_t *iw) {
    ncwrap_error error = NCW_OK;
    if (OK != delwin((*iw)->window)) {
        error = NCW_NCURSES_FAIL;
    }
    free((void *)*iw);
    *iw = NULL;
    return error;
}

void delete(char *buf, size_t bufsz, int idx) {
    for (size_t i = 0; i < bufsz; ++i) {
        if (i >= idx) {
            buf[i] = buf[i + 1];
        }
    }
}

void
insert(char *buf, size_t bufsz, int idx, char ch) {
    for (size_t i = bufsz - 1; i > idx; --i) {
        if (i > idx) {
            buf[i] = buf[i - 1];
        }
    }
    buf[idx] = ch;
}

ncwrap_error
input_window_read(input_window_t iw, char *buf, size_t bufsz) {
    // display cursor at the input line
    wmove(iw->window, 1, 1);
    curs_set(1);
    wrefresh(iw->window);

    int scope = 0;
    int next, cursor = 0;
    int width = iw->width;
    char display[width];
    int capture = 0;
    for (size_t i = 0; i < bufsz - 1; ++i) {
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
                delete (buf, bufsz, scope + cursor - 1);
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
            buf[i] = '\0';
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
                insert(buf, bufsz, scope + cursor, (char)next);
                if (width - 3 == cursor) {
                    scope += 1;
                } else {
                    cursor += 1;
                }

            } else
                break;
        }

        buf[i + 1] = '\0';
        window_content_clear(iw->window, iw->title);

        // only need to display a portion of the buffer
        for (int j = 0; j < width - 1; ++j) {
            display[j] = buf[scope + j];
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

ncwrap_error
scroll_window_init(scroll_window_t *sw, int x, int y, int width, int height,
                   const char *title) {

    ncwrap_error error = NCW_OK;

    // store title right after window struct
    *sw = (scroll_window_t)malloc(sizeof(struct scroll_window) + strlen(title) +
                                  1);
    if (NULL == *sw) {
        error = NCW_INSUFFICIENT_MEMORY;
        goto end;
    }

    (*sw)->title = (char *)(*sw + 1);
    (void)strncpy((*sw)->title, title, strlen(title) + 1);

    // create window
    (*sw)->window = newwin(height, width, y, x);
    if (NULL == (*sw)->window) {
        error = NCW_NCURSES_FAIL;
        goto clean;
    }

    (*sw)->width = width;
    (*sw)->height = height;

    if (OK != box((*sw)->window, 0, 0)) {
        error = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    if (OK != mvwprintw((*sw)->window, 0, 1, " %s ", title)) {
        error = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    if (OK != wrefresh((*sw)->window)) {
        error = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    // enable scrolling in this window
    scrollok((*sw)->window, TRUE);

    goto end;

cleanall:
    delwin((*sw)->window);
clean:
    free((void *)*sw);
    *sw = NULL;
end:
    return error;
}

ncwrap_error
scroll_window_close(scroll_window_t *sw) {
    ncwrap_error error = NCW_OK;
    if (OK != delwin((*sw)->window)) {
        error = NCW_NCURSES_FAIL;
    }
    free((void *)*sw);
    *sw = NULL;
    return error;
}

ncwrap_error
scroll_window_add_line(scroll_window_t sw, const char *line) {

    ncwrap_error error = NCW_OK;

    if (NULL == line) {
        error = NCW_INVALID_PARAM;
        goto end;
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

end:
    return error;
}

ncwrap_error
menu_window_init(menu_window_t *mw, int x, int y, int width, int height,
                 const char *title) {

    ncwrap_error error = NCW_OK;

    // store title right after the struct
    *mw = (menu_window_t)malloc(sizeof(struct menu_window) + strlen(title) + 1);
    if (NULL == *mw) {
        error = NCW_INSUFFICIENT_MEMORY;
        goto end;
    }

    (*mw)->title = (char *)(*mw + 1);
    (void)strncpy((*mw)->title, title, strlen(title) + 1);

    // create window
    (*mw)->window = newwin(height, width, y, x);
    if (NULL == (*mw)->window) {
        error = NCW_NCURSES_FAIL;
        goto clean;
    }

    (*mw)->width = width;
    (*mw)->height = height;
    (*mw)->options = (option_t *)NULL;
    (*mw)->options_num = 0;
    (*mw)->highlight = 0;

    if (OK != box((*mw)->window, 0, 0)) {
        error = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    if (OK != mvwprintw((*mw)->window, 0, 1, " %s ", title)) {
        error = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    if (OK != wrefresh((*mw)->window)) {
        error = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    // enable scrolling in this window
    scrollok((*mw)->window, TRUE);

    goto end;

cleanall:
    delwin((*mw)->window);
clean:
    free((void *)*mw);
    *mw = NULL;
end:
    return error;
}

ncwrap_error
menu_window_close(menu_window_t *mw) {
    ncwrap_error error = NCW_OK;
    if (OK != delwin((*mw)->window)) {
        error = NCW_NCURSES_FAIL;
    }
    for (int i = 0; i < (*mw)->options_num; ++i) {
        free((void *)((*mw)->options + i)->label);
    }
    free((void *)(*mw)->options);
    free((void *)*mw);
    mw = NULL;
    return error;
}

ncwrap_error
menu_window_update(menu_window_t mw) {
    ncwrap_error error = NCW_OK;
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
    return error;
}

ncwrap_error
menu_window_add_option(menu_window_t mw, const char *label, void (*cb)(void *),
                       void *ctx) {

    ncwrap_error error = NCW_OK;

    for (int i = 0; i < mw->options_num; ++i) {
        if (strcmp(mw->options[i].label, label) == 0) {
            return error;
        }
    }

    // alocate place for the new option
    option_t *new_options = (option_t *)realloc(
        mw->options, (mw->options_num + 1) * sizeof(option_t));
    if (NULL == new_options) {
        ncwrap_handle_error;
        return error;
    }
    mw->options = new_options;

    // init new option
    option_t *new_option = mw->options + mw->options_num;
    new_option->label = (char *)malloc(strlen(label) + 1);
    if (NULL == new_option->label) {
        ncwrap_handle_error;
        return error;
    }

    (void)strncpy(new_option->label, label, strlen(label) + 1);
    new_option->cb = cb;
    new_option->ctx = ctx;

    mw->options_num += 1;
    menu_window_update(mw);

    return error;
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

ncwrap_error
menu_window_delete_option(menu_window_t mw, const char *label) {

    ncwrap_error error = NCW_OK;

    for (int i = 0; i < mw->options_num; ++i) {
        if (strcmp((mw->options + i)->label, label) == 0) {
            squash(mw, i);

            void *new_options =
                realloc(mw->options, (mw->options_num - 1) * sizeof(option_t));
            if (NULL == new_options) {
                ncwrap_handle_error;
                return error;
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

    return error;
}

ncwrap_error
menu_window_start(menu_window_t mw) {

    ncwrap_error error = NCW_OK;

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

    return error;
}

void
window_content_clear(WINDOW *window, char *title) {
    werase(window);
    box(window, 0, 0);
    mvwprintw(window, 0, 1, " %s ", title);
}
