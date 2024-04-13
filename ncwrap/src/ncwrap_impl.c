#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ncurses.h"

#include "ncwrap.h"

#include "ncwrap_helper.h"
#include "ncwrap_impl.h"

// VISUAL--------------------------------------------
//    TODO: indicate nothing happened input window (retval?)
//    TODO: indicate nothing happened menu window (retval?)
//    TODO: enable resize of window
//    TODO: add box border art
// --------------------------------------------------

// BUG-----------------------------------------------
//    TODO: set visibility on functions
// --------------------------------------------------

// FEATURE-------------------------------------------
//    TODO: add main event loop using nodelay, to draw,input
//    TODO: err code interpreter fuction on interface
//    TODO: logging with dlt or syslog or stderr
//    TODO: introduce thread safety ??
//    TODO: terminal window
//    TODO: debug window
//    TODO: game window
//    TODO: custom window
//    TODO: window editor window
//    TODO: save log from scroll window
//    TODO: login window
//    TODO: header menus for windows
//    TODO: macro keys for windows that call callbacks
//    TODO: err message box
// --------------------------------------------------

ncw_err
ncw_init(void) {

    ncw_err err = NCW_OK;

    /* init ncurses library */
    stdscr = initscr();
    if (NULL == stdscr) {
        goto fail;
    }

    /* tell ncurses not to do NL->CR/NL on output */
    if (OK != nonl()) {
        goto fail;
    }

    /* take input chars one at a time, no wait for '\n' */
    if (OK != cbreak()) {
        goto fail;
    }

    /* echo input - in color */
    if (OK != echo()) {
        goto fail;
    }

    /* set cursor to invisible  */
    if (ERR == curs_set(0)) {
        goto fail;
    }

end:
    return err;

fail:
    err = NCW_NCURSES_FAIL;
    goto end;
}

ncw_err
ncw_close(void) {

    ncw_err err = NCW_OK;

    /* terminate ncurses library */
    if (OK != endwin()) {
        goto fail;
    }

end:
    return err;

fail:
    err = NCW_NCURSES_FAIL;
    goto end;
}

ncw_err
ncw_input_window_init(input_window_t *iw, int x, int y, int width,
                      const char *title) {

    ncw_err err = NCW_OK;

    if (NULL == title) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    // this really should not be an input_window_t since there is a variable
    // length string stored after the struct but its fine for now
    *iw =
        (input_window_t)malloc(sizeof(struct input_window) + strlen(title) + 1);
    if (NULL == *iw) {
        err = NCW_INSUFFICIENT_MEMORY;
        goto end;
    }

    // store title right after the window struct
    (*iw)->title = (char *)(*iw + 1);
    (void)safe_strncpy((*iw)->title, title, strlen(title) + 1);

    // create the window entity with fix 3 height
    (*iw)->window = newwin(3, width, y, x);
    if (NULL == (*iw)->window) {
        err = NCW_NCURSES_FAIL;
        goto clean;
    }

    (*iw)->width = width;

    if (OK != window_draw_box((*iw)->window, title)) {
        err = NCW_NCURSES_FAIL;
        goto cleanall;
    }

end:
    return err;

cleanall:
    delwin((*iw)->window);

clean:
    free((void *)*iw);
    *iw = NULL;
    goto end;
}

ncw_err
ncw_input_window_close(input_window_t *iw) {

    ncw_err err = NCW_OK;

    if (NULL == *iw) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    if (OK != delwin((*iw)->window)) {
        err = NCW_NCURSES_FAIL;
        goto end;
    }

    free((void *)*iw);
    *iw = NULL;

end:
    return err;
}

ncw_err
ncw_input_window_read(input_window_t iw, char *buf, size_t bufsz) {

    ncw_err err = NCW_OK;

    int width = iw->width;
    char display[width];

    if (NULL == buf || 0 == bufsz) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    int scope = 0;
    int next, cursor = 0;
    int capture = 0;

    // display cursor at the input line
    if (OK != wmove(iw->window, 1, 1)) {
        goto fail;
    }

    if (ERR == curs_set(1)) {
        err = NCW_NCURSES_FAIL;
        goto fail;
    }

    if (OK != wrefresh(iw->window)) {
        err = NCW_NCURSES_FAIL;
        goto fail;
    }

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
                del(buf, bufsz, scope + cursor - 1);
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
            goto clean;

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
                ins(buf, bufsz, scope + cursor, (char)next);
                if (width - 3 == cursor) {
                    scope += 1;
                } else {
                    cursor += 1;
                }

            } else
                break;
        }

        buf[i + 1] = '\0';

        if (OK != window_content_clear(iw->window, iw->title)) {
            goto fail;
        }

        // only need to display a portion of the bmffer
        for (int j = 0; j < width - 1; ++j) {
            display[j] = buf[scope + j];
            if (j == iw->width - 2) {
                display[j] = '\0';
            };
        }

        // print the portion that should be visible
        if (OK != mvwprintw(iw->window, 1, 1, "%s", display)) {
            goto fail;
        }

        // left indicator
        if (scope != 0) {
            if (OK != mvwprintw(iw->window, 1, 0, "<")) {
                goto fail;
            }
        }

        // right indicator
        if (i - scope + 1 >= width - 2) {
            if (OK != mvwprintw(iw->window, 1, width - 1, ">")) {
                goto fail;
            }
        }

        // move the cursor to the appropriate position
        if (OK != wmove(iw->window, 1, 1 + cursor)) {
            goto fail;
        }
    }

clean:
    if (OK != window_clear(iw->window)) {
        goto fail;
    }

end:
    curs_set(0);
    return err;

fail:
    err = NCW_NCURSES_FAIL;
    goto end;
}

ncw_err
ncw_scroll_window_init(scroll_window_t *sw, int x, int y, int width, int height,
                       const char *title) {

    ncw_err err = NCW_OK;

    if (NULL == title) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    // store title right after window struct
    *sw = (scroll_window_t)malloc(sizeof(struct scroll_window) + strlen(title) +
                                  1);
    if (NULL == *sw) {
        err = NCW_INSUFFICIENT_MEMORY;
        goto end;
    }

    (*sw)->title = (char *)(*sw + 1);
    (void)safe_strncpy((*sw)->title, title, strlen(title) + 1);

    // create window
    (*sw)->window = newwin(height, width, y, x);
    if (NULL == (*sw)->window) {
        err = NCW_NCURSES_FAIL;
        goto clean;
    }

    (*sw)->width = width;
    (*sw)->height = height;

    if (OK != window_draw_box((*sw)->window, title)) {
        err = NCW_NCURSES_FAIL;
        goto cleanall;
    }

    // enable scrolling in this window
    scrollok((*sw)->window, TRUE);

end:
    return err;

cleanall:
    delwin((*sw)->window);

clean:
    free((void *)*sw);
    *sw = NULL;
    goto end;
}

ncw_err
ncw_scroll_window_close(scroll_window_t *sw) {

    ncw_err err = NCW_OK;

    if (NULL == *sw) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    if (OK != delwin((*sw)->window)) {
        err = NCW_NCURSES_FAIL;
        goto end;
    }

    free((void *)*sw);
    *sw = NULL;

end:
    return err;
}

ncw_err
ncw_scroll_window_add_line(scroll_window_t sw, const char *line) {

    ncw_err err = NCW_OK;

    if (NULL == line) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    // displace all lines 1 up (literally)
    if (OK != scroll(sw->window)) {
        goto fail;
    }

    // clear the place of the added line
    if (OK != wmove(sw->window, sw->height - 2, 1)) {
        goto fail;
    }

    if (OK != wclrtoeol(sw->window)) {
        goto fail;
    }

    // print the line in the correct place
    if (OK != mvwprintw(sw->window, sw->height - 2, 1, "%s", line)) {
        goto fail;
    }

    // reconstruct the widnow box
    if (OK != window_draw_box(sw->window, sw->title)) {
        goto fail;
    }

end:
    return err;

fail:
    err = NCW_NCURSES_FAIL;
    goto end;
}

ncw_err
ncw_menu_window_init(menu_window_t *mw, int x, int y, int width, int height,
                     const char *title) {

    ncw_err err = NCW_OK;

    if (NULL == title) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    // store title right after the struct
    *mw = (menu_window_t)malloc(sizeof(struct menu_window) + strlen(title) + 1);
    if (NULL == *mw) {
        err = NCW_INSUFFICIENT_MEMORY;
        goto end;
    }

    (*mw)->title = (char *)(*mw + 1);
    (void)safe_strncpy((*mw)->title, title, strlen(title) + 1);

    // create window
    (*mw)->window = newwin(height, width, y, x);
    if (NULL == (*mw)->window) {
        err = NCW_NCURSES_FAIL;
        goto clean;
    }

    (*mw)->width = width;
    (*mw)->height = height;
    (*mw)->options = (option_t *)NULL;
    (*mw)->options_num = 0;
    (*mw)->highlight = 0;

    if (OK != window_draw_box((*mw)->window, title)) {
        err = NCW_NCURSES_FAIL;
        goto cleanall;
    }

end:
    return err;

cleanall:
    delwin((*mw)->window);

clean:
    free((void *)*mw);
    *mw = NULL;
    goto end;
}

ncw_err
ncw_menu_window_close(menu_window_t *mw) {

    ncw_err err = NCW_OK;

    if (NULL == *mw) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    if (OK != delwin((*mw)->window)) {
        err = NCW_NCURSES_FAIL;
        goto end;
    }

    for (int i = 0; i < (*mw)->options_num; ++i) {
        free((void *)((*mw)->options + i)->label);
    }

    free((void *)(*mw)->options);
    free((void *)*mw);
    *mw = NULL;

end:
    return err;
}

ncw_err
menu_window_update(menu_window_t mw) {

    ncw_err err = NCW_OK;

    if (OK != window_content_clear(mw->window, mw->title)) {
        goto fail;
    }

    for (int i = 0; i < mw->options_num; ++i) {

        if (mw->highlight == i) {
            if (OK != wattron(mw->window, A_REVERSE)) {
                goto fail;
            }
        }

        if (OK !=
            mvwprintw(mw->window, i + 1, 1, "%s", (mw->options + i)->label)) {
            goto fail;
        }

        if (mw->highlight == i) {
            if (OK != wattroff(mw->window, A_REVERSE)) {
                goto fail;
            }
        }
    }

    if (OK != wrefresh(mw->window)) {
        goto fail;
    }

end:
    return err;

fail:
    err = NCW_NCURSES_FAIL;
    goto end;
}

ncw_err
ncw_menu_window_add_option(menu_window_t mw, const char *label,
                           void (*cb)(void *), void *ctx) {

    ncw_err err = NCW_OK;

    if (strlen(label) > mw->width - 2) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    for (int i = 0; i < mw->options_num; ++i) {
        if (strcmp(mw->options[i].label, label) == 0) {
            err = NCW_INVALID_PARAM;
            goto end;
        }
    }

    // alocate place for the new option
    option_t *new_options = (option_t *)realloc(
        mw->options, (mw->options_num + 1) * sizeof(option_t));
    if (NULL == new_options) {
        err = NCW_INSUFFICIENT_MEMORY;
        goto end;
    }

    mw->options = new_options;

    // init new option
    option_t *new_option = mw->options + mw->options_num;
    new_option->label = (char *)malloc(strlen(label) + 1);
    if (NULL == new_option->label) {
        err = NCW_INSUFFICIENT_MEMORY;
        goto end;
    }

    (void)safe_strncpy(new_option->label, label, strlen(label) + 1);
    new_option->cb = cb;
    new_option->ctx = ctx;

    mw->options_num += 1;

    err = menu_window_update(mw);

end:
    return err;
}

ncw_err
ncw_menu_window_delete_option(menu_window_t mw, const char *label) {

    ncw_err err = NCW_OK;

    if (NULL == label) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    for (int i = 0; i < mw->options_num; ++i) {
        if (strcmp((mw->options + i)->label, label) == 0) {

            squash(mw, i);

            void *new_options =
                realloc(mw->options, (mw->options_num - 1) * sizeof(option_t));
            if (NULL == new_options) {
                err = NCW_INSUFFICIENT_MEMORY;
                goto end;
            }

            mw->options = new_options;
            --mw->options_num;

            // adjust highlight
            if (i <= mw->highlight) {
                --mw->highlight;
            }

            err = menu_window_update(mw);
            goto end;
        }
    }

end:
    return err;
}

ncw_err
ncw_menu_window_start(menu_window_t mw) {

    ncw_err err = NCW_OK;

    int ch;
    while (true) {
        while (true) {
            if (mw->highlight >= mw->options_num) {
                --mw->highlight;
            } else {
                break;
            }
        }

        err = menu_window_update(mw);
        if (err != NCW_OK) {
            goto end;
        }

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
            err = menu_window_update(mw);
            goto end;

        default:;
        }
    }

end:
    return err;
}
