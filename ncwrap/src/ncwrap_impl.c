// VISUAL--------------------------------------------
//    TODO: indicate nothing happened input window (retval?)
//    TODO: indicate nothing happened menu window (retval?)
//    TODO: enable resize of window
//    TODO: add box border art
// --------------------------------------------------

// BUG-----------------------------------------------
//    TODO: menu window refresh only if needed (glitches)
//    TODO: separate different type of widnows into different files
//    TODO: set visibility on functions
//    TODO: refresh optimization
//    https://www.man7.org/linux/man-pages/man3/curs_refresh.3x.html
// --------------------------------------------------

// FEATURE-------------------------------------------
//    TODO: add main event loop using nodelay, to draw,input
//    by defining a common UPDATE and HANDLE callback that all windows
//    can implement. Update defines how to refresh the window
//    and handle gets called depending on which window is in focus.
//    Handle can decide what to do with the character. The main
//    event loop needs a the wgetch that is non-blocking.
//    each window has an update callback that is registered when INIT
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

static window_t *_window = NULL;

ncw_err
ncw_init(void) {

    ncw_err err = NCW_OK;

    /* init ncurses library */
    stdscr = initscr();
    if (NULL == stdscr) {
        goto fail;
    }

    /* refresh the main screen */
    if (ERR == refresh()) {
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

    /* set cursor to invisible  */
    if (ERR == curs_set(0)) {
        goto fail;
    }

    /* disble echo to windows */
    if (OK != noecho()) {
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

window_t *
_window_register(update_t update, handler_t handler) {

    // malloc new element
    window_t *new = (window_t *)malloc(sizeof(window_t));
    if (NULL == new) {
        return (window_t *)NULL;
    }

    // init new element
    new->update = update;
    new->handler = handler;

    // if empty
    if (NULL == _window) {
        // if first in _window
        new->prev = new;
        new->next = new;
        _window = new;
    } else {
        // if not first init prev
        _window->prev->next = new;
        new->prev = _window->prev;

        _window->prev = new;
        new->next = _window;
    }

    return new;
}

void
_window_unregister(window_t *window_handle) {

    // invalid pointer
    if (NULL == window_handle) {
        return;
    }

    // _window points here
    if (_window == window_handle) {

        // this is the last element
        if (_window == window_handle->prev &&
            window_handle->prev == window_handle->next) {
            _window = NULL;
            goto clean;

        } else {
            _window = window_handle->next;
        }
    }

    // _window points somewhere else
    window_handle->prev->next = window_handle->next;
    window_handle->next->prev = window_handle->prev;

clean:
    free(window_handle);
}

void
_window_update(void) {

    if (NULL == _window) {
        return;
    }

    for (window_t *iter = _window;; iter = iter->next) {
        iter->update.cb(iter->update.ctx);
        if (_window == iter->next) {
            break;
        }
    }

    return;
}

ncw_err
ncw_start(void) {

    int event = 0;
    window_t *_window_focus = _window->next->next;

    for (;;) {

        _window_update();
        event = wgetch(stdscr);
        switch (event) {

        case CTRL('n'):
            // notify window of focus change
            if (NULL != _window_focus->handler.cb) {
                _window_focus->handler.cb('q', _window_focus->handler.ctx);
            }
            _window_focus = _window_focus->next;
            break;

        case CTRL('x'):
            goto end;

        default:
            if (NULL != _window_focus->handler.cb) {
                _window_focus->handler.cb(event, _window_focus->handler.ctx);
            }
        }
    }

end:
    return NCW_OK;
}

ncw_err
input_window_update(void *window_ctx);

ncw_err
input_window_handler(int event, void *window_ctx);

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

    // create structs for the event handler registration
    update_t update = {.cb = input_window_update, .ctx = (void *)iw};
    handler_t handler = {.cb = input_window_handler, .ctx = (void *)iw};

    // register window for the event handler
    (*iw)->_window = _window_register(update, handler);
    if (NULL == (*iw)->_window) {
        err = NCW_INSUFFICIENT_MEMORY;
        goto cleanall;
    }

    (*iw)->width = width;

    // set output
    (*iw)->cb = NULL;
    (*iw)->ctx = NULL;

    (*iw)->buf = (char *)malloc(_BUFSZ);
    if (NULL == (*iw)->buf) {
        err = NCW_INSUFFICIENT_MEMORY;
        goto unreg;
    }

    (*iw)->buf_sz = _BUFSZ;
    (*iw)->line_sz = 0;
    (*iw)->display_offs = 0;
    (*iw)->cursor_offs = 0;

end:
    return err;
unreg:
    _window_unregister((*iw)->_window);
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

    if (OK != window_clear((*iw)->window)) {
        err = NCW_NCURSES_FAIL;
        goto end;
    }

    if (OK != delwin((*iw)->window)) {
        err = NCW_NCURSES_FAIL;
        goto end;
    }

    _window_unregister((*iw)->_window);

    free((void *)(*iw)->buf);
    free((void *)*iw);
    *iw = NULL;

end:
    return err;
}

ncw_err
ncw_input_window_set_output(input_window_t iw, output_cb cb, void *ctx) {

    if (NULL == cb || NULL == ctx) {
        return NCW_INVALID_PARAM;
    }

    iw->cb = cb;
    iw->ctx = ctx;

    return NCW_OK;
}

ncw_err
input_window_update(void *window_ctx) {

    ncw_err err = NCW_OK;
    input_window_t iw = *((input_window_t *)window_ctx);

    if (0 != iw->line_sz) {

        char *display = (char *)malloc(iw->line_sz - iw->display_offs - 1);
        if (NULL == display) {
            err = NCW_INSUFFICIENT_MEMORY;
            goto end;
        }

        for (size_t i = 0; i < iw->line_sz - iw->display_offs - 1; ++i) {
            display[i] = iw->buf[iw->display_offs + i];
        }

        display[iw->line_sz - iw->display_offs] = '\0';

        if (OK != window_content_clear(iw->window, iw->title)) {
            free((void *)display);
            goto fail;
        }

        // print the portion that should be visible
        if (OK != mvwprintw(iw->window, 1, 1, "%s", display)) {
            free((void *)display);
            goto fail;
        }

        free((void *)display);

    } else {

        if (OK != window_content_clear(iw->window, iw->title)) {
            goto fail;
        }
    }

    // left indicator
    if (iw->display_offs != 0) {
        if (OK != mvwprintw(iw->window, 1, 0, "<")) {
            goto fail;
        }
    }

    // right indicator
    if (iw->line_sz - iw->display_offs + 1 > iw->width - 2) {
        if (OK != mvwprintw(iw->window, 1, iw->width - 1, ">")) {
            goto fail;
        }
    }

    // move the cursor to the appropriate position
    if (OK != wmove(iw->window, 1, 1 + iw->cursor_offs)) {
        goto fail;
    }

    if (OK != wrefresh(iw->window)) {
        goto fail;
    }

end:
    return err;
fail:
    err = NCW_NCURSES_FAIL;
    goto end;
}

ncw_err
input_window_handler(int event, void *window_ctx) {

    ncw_err err = NCW_OK;
    input_window_t iw = *((input_window_t *)window_ctx);

    // ignore control ascii
    if (event < 32 || event > 127) {
        goto end;
    }

    // if (iw->line_sz == iw->buf_sz - 1) {
    //     if (iw->line_sz < _BUFSZMAX) {

    //         char tmp[iw->buf_sz];
    //         (void)safe_strncpy(tmp, iw->buf, iw->buf_sz);

    //         iw->buf = realloc(iw->buf, iw->buf_sz);
    //         if (NULL == iw->buf) {
    //             err = NCW_INSUFFICIENT_MEMORY;
    //             goto end;
    //         }
    //     } else {
    //         goto end;
    //     }
    // }

    // draw_cursor
    if (OK != (wmove(iw->window, 1, 0) + curs_set(1) + wrefresh(iw->window))) {
        err = NCW_NCURSES_FAIL;
        goto end;
    }

    switch (event) {
    case 127: //< backspace
    case KEY_DC:
    case KEY_BACKSPACE:

        // if there are characters before the cursor that can be deleted
        if (iw->display_offs != 0 || iw->cursor_offs != 0) {

            del(iw->buf, iw->buf_sz, iw->display_offs + iw->cursor_offs - 1);

            if (iw->display_offs != 0 &&
                (iw->cursor_offs + iw->display_offs == iw->line_sz ||
                 iw->cursor_offs <= iw->width)) {

                iw->display_offs -= 1;

            } else {
                iw->cursor_offs -= 1;
            }

            iw->line_sz -= 2;

        } else { /* indicate nothing happened */
        }

        break;

    case '\n': //< return
    case '\r':
    case KEY_ENTER:

        iw->buf[iw->line_sz] = '\0';

        iw->cb(iw->buf, iw->buf_sz, iw->ctx);
        free((void *)iw->buf);
        iw->buf = NULL;
        iw->buf_sz = 0;
        iw->line_sz = 0;
        iw->display_offs = 0;
        iw->cursor_offs = 0;

        break;

    case 27: //< escape

        wgetch(stdscr);
        switch (wgetch(stdscr)) {

        case 'D': //< left arrow
        case KEY_LEFT:

            if (iw->cursor_offs != 0) {
                iw->cursor_offs -= 1;
            } else {
                if (iw->display_offs != 0) {
                    iw->display_offs -= 1;
                }
            }
            break;

        case 'C': //< right arrow
        case KEY_RIGHT:

            if (iw->cursor_offs != iw->width - 3) {
                if (iw->cursor_offs < iw->line_sz) {
                    iw->cursor_offs += 1;
                }
            } else if (iw->cursor_offs + iw->display_offs != iw->line_sz) {
                iw->display_offs += 1;
            }
            break;

        case 'A': //< up arrow
        case 'B': //< down arrow
        case KEY_UP:
        case KEY_DOWN:
            break;

        default:;
        }
        break;

    default:
        printf("kaka");
        // ins(iw->buf, iw->buf_sz, iw->display_offs + iw->cursor_offs,
        // (char)event);

        if (iw->width - 1 == iw->cursor_offs) {
            iw->display_offs += -1;
        } else {
            iw->cursor_offs += -1;
        }
    }

end:
    if (OK != curs_set(0)) {
        err = NCW_NCURSES_FAIL;
    }

    return err;
}

ncw_err
scroll_window_update(void *window_ctx);

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

    update_t update = {.cb = scroll_window_update, .ctx = (void *)sw};
    handler_t handler = {.cb = NULL, .ctx = NULL};

    // register window for the event handler
    (*sw)->_window = _window_register(update, handler);
    if (NULL == (*sw)->_window) {
        err = NCW_INSUFFICIENT_MEMORY;
        goto cleanall;
    }

    (*sw)->width = width;
    (*sw)->height = height;
    (*sw)->next_line = NULL;

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

    if (OK != window_clear((*sw)->window)) {
        err = NCW_INVALID_PARAM;
        goto end;
    }

    if (OK != delwin((*sw)->window)) {
        err = NCW_NCURSES_FAIL;
        goto end;
    }

    _window_unregister((*sw)->_window);

    free((void *)(*sw)->next_line);
    free((void *)*sw);
    *sw = NULL;

end:
    return err;
}

ncw_err
scroll_window_update(void *window_ctx) {

    ncw_err err = NCW_OK;
    scroll_window_t sw = *((scroll_window_t *)window_ctx);

    // if there is no new next line, skip
    if (NULL == sw->next_line) {
        goto skip;
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
    if (OK != mvwprintw(sw->window, sw->height - 2, 1, "%s", sw->next_line)) {
        goto fail;
    }

    // free next line for new one
    free((void *)sw->next_line);
    sw->next_line = NULL;

skip:
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
ncw_scroll_window_add_line(scroll_window_t sw, const char *line) {

    if (NULL != sw->next_line) {
        free(sw->next_line);
    }

    sw->next_line = (char *)malloc(strlen(line) + 1);
    if (NULL == sw->next_line) {
        return NCW_INSUFFICIENT_MEMORY;
    }
    (void)safe_strncpy(sw->next_line, line, strlen(line) + 1);

    return NCW_OK;
}

ncw_err
menu_window_update(void *window_ctx);

ncw_err
menu_window_handler(int event, void *window_ctx);

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

    update_t update = {.cb = menu_window_update, .ctx = (void *)mw};
    handler_t handler = {.cb = menu_window_handler, .ctx = (void *)mw};

    // register window for the event handler
    (*mw)->_window = _window_register(update, handler);
    if (NULL == (*mw)->_window) {
        goto cleanall;
    }

    (*mw)->width = width;
    (*mw)->height = height;
    (*mw)->options = (option_t *)NULL;
    (*mw)->options_num = 0;
    (*mw)->highlight = 0;

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

    // if (OK != window_clear((*mw)->window)) {
    //     err = NCW_NCURSES_FAIL;
    //     goto end;
    // }

    if (OK != delwin((*mw)->window)) {
        err = NCW_NCURSES_FAIL;
        goto end;
    }

    _window_unregister((*mw)->_window);

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
menu_window_update(void *window_ctx) {

    ncw_err err = NCW_OK;
    menu_window_t mw = *((menu_window_t *)window_ctx);

    // adjust highlight
    while (true) {
        if (mw->highlight >= mw->options_num) {
            --mw->highlight;
        } else {
            break;
        }
    }

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

            goto end;
        }
    }

end:
    return err;
}

ncw_err
menu_window_handler(int event, void *window_ctx) {

    ncw_err err = NCW_OK;
    menu_window_t mw = *((menu_window_t *)window_ctx);

    switch (event) {
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
        (mw->options + mw->highlight)->cb((mw->options + mw->highlight)->ctx);
        break;

    // do nothing for arrow characters
    case 27:            //< Esc
        wgetch(stdscr); //< swallow next character
        wgetch(stdscr); //< A,B,C,D arrow
        break;

    // stop highlighting
    case 113: //< q
        mw->highlight = -1;
        break;

    default:;
    }

    return err;
}
