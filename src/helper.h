#ifndef _NCWRAP_HELPER_H_HEADER_GUARD
#define _NCWRAP_HELPER_H_HEADER_GUARD

#include "ncurses.h"
#include "ncwrap.h"

#define _BUFSZ 32
#define _BUFSZMAX 256

#define FOCUS_ON (-254)
#define FOCUS_OFF (-255)

char *
safe_strncpy(char *dst, const char *src, size_t size);

void
del(char *buf, size_t bufsz, int idx);

void
ins(char *buf, size_t bufsz, int idx, char ch);

void
squash(menu_window_t mw, int option_offset);

int
window_draw_box(WINDOW *window, const char *title);

int
window_clear(WINDOW *window);

int
window_content_clear(WINDOW *window, const char *title);

#endif // _NCWRAP_HELPER_H_HEADER_GUARD

