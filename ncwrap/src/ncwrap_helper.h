#ifndef NCWRAP_HELPER_H_HEADER_GUARD
#define NCWRAP_HELPER_H_HEADER_GUARD

#include "ncwrap_impl.h"

char *
safe_strncpy(char *dst, const char *src, size_t size) {
    dst[size - 1] = '\0';
    return strncpy(dst, src, size - 1);
}

void
del(char *buf, size_t bufsz, int idx) {

    for (size_t i = 0; i < bufsz; ++i) {
        if (i >= idx) {
            buf[i] = buf[i + 1];
        }
    }
}

void
ins(char *buf, size_t bufsz, int idx, char ch) {

    for (size_t i = bufsz - 1; i > idx; --i) {
        if (i > idx) {
            buf[i] = buf[i - 1];
        }
    }

    buf[idx] = ch;
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

int
window_draw_box(WINDOW *window, const char *title) {

    int error = OK;
    error = box(window, 0, 0);
    if (OK != error) {
        goto end;
    }

    error = mvwprintw(window, 0, 1, " %s ", title);
    if (OK != error) {
        goto end;
    }

    error = wrefresh(window);
    if (OK != error) {
        goto end;
    }

end:
    return error;
}

int
window_clear(WINDOW *window) {

    int error = OK;
    werase(window);
    if (OK != error) {
        goto end;
    }

    wrefresh(window);
    if (OK != error) {
        goto end;
    }

end:
    return error;
}

int
window_content_clear(WINDOW *window, const char *title) {

    int error = OK;
    window_clear(window);
    if (OK != error) {
        goto end;
    }

    window_draw_box(window, title);
    if (OK != error) {
        goto end;
    }

end:
    return error;
}

#endif // NCWRAP_HELPER_H_HEADER_GUARD
