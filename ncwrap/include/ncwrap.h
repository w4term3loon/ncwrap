#ifndef NCWRAP_H_HEADER_GUARD
#define NCWRAP_H_HEADER_GUARD

#include <stddef.h>

// input window type
typedef struct input_window_t *input_window_t;

// scroll window type
typedef struct scroll_window_t *scroll_window_t;

// menu window type
typedef struct menu_window_t *menu_window_t;

/* initialize the library
 * always call before the first window call
 */
void
ncwrap_init();

/* deinitialize the library
 * always call after the last window call
 */
void
ncwrap_close();

/* Initialize input window with dimensions and title.
 * @param x: the horizontal position of the top-left corner.
 * @param y: the vertical position of the top-left corner.
 * @param width: the width of the window.
 * @param title: the title of the window.
 * @return: the initialized window.
 */
input_window_t
input_window_init(int x, int y, int width, const char *title);

/* Close input window.
 * @param input_window: the window to be closed.
 */
void
input_window_close(input_window_t input_window);

/* Read string from input window.
 * @param input_window: the target window.
 * @param buff: the buffer to store the string in.
 * @param buff_sz: the size of the read string.
 * @return: TODO.
 */
int
input_window_read(input_window_t input_window, char *buff, size_t buff_sz);

/* Initialize scroll window with dimensions and title.
 * @param x: the horizontal position of the top-left corner.
 * @param y: the vertical position of the top-left corner.
 * @param width: the width of the window.
 * @param height: the height of the window.
 * @param title: the title of the window.
 * @return: the initialized window.
 */
scroll_window_t
scroll_window_init(int x, int y, int width, int height, const char *title);

/* Close scroll window.
 * @param scroll_window: the target window.
 */
void
scroll_window_close(scroll_window_t scroll_window);

/* Add line to the scroll window.
 * @param scroll_window: the target window.
 * @param line: the line to be added to the window.
 */
void
scroll_window_add_line(scroll_window_t scroll_window, const char *line);

// option callback type
typedef void (*option_cb)(void *ctx);

/* Initialize menu window with dimensions and title.
 * @param x: the horizontal position of the top-left corner.
 * @param y: the vertical position of the top-left corner.
 * @param width: the width of the window.
 * @param height: the height of the window.
 * @param title: the title of the window.
 * @return: the initialized window.
 */
menu_window_t
menu_window_init(int x, int y, int width, int height, const char *title);

/* Close menu window.
 * @param menu_window: the target window.
 */
void
menu_window_close(menu_window_t menu_window);

/* Start event loop in the menu window.
 * @param menu_window: the target window.
 */
void
menu_window_start(menu_window_t menu_window);

/* Add option to the menu window.
 * @param menu_window: the target window.
 * @param label: the name of the new option.
 * @param cb: the callback of the new option.
 * @param ctx: the context of the callback.
 */
void
menu_window_add_option(menu_window_t menu_window, const char *label,
                       option_cb cb, void *ctx);

/* Delete option from the menu window.
 * @param menu_window: the target window.
 * @param label: the name of the option.
 */
void
menu_window_delete_option(menu_window_t menu_window, const char *label);

#endif // NCWRAP_H_HEADER_GUARD
