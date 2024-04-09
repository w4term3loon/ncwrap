#ifndef NCWRAP_H_HEADER_GUARD
#define NCWRAP_H_HEADER_GUARD

#include <stddef.h>

typedef enum {
    NCW_OK = 0,
    NCW_INVALID_PARAM,
} ncwrap_error;

// input window type
typedef struct input_window *input_window_t;

// scroll window type
typedef struct scroll_window *scroll_window_t;

// menu window type
typedef struct menu_window *menu_window_t;

/* initialize the library
 * always call before the first window call
 */
ncwrap_error
ncwrap_init();

/* deinitialize the library
 * always call after the last window call
 */
ncwrap_error
ncwrap_close();

/* Initialize input window with dimensions and title.
 * @param iw[out]: the initialized window.
 * @param x[in]: the horizontal position of the top-left corner.
 * @param y[in]: the vertical position of the top-left corner.
 * @param width[in]: the width of the window.
 * @param title[in]: the title of the window.
 * @return: error code.
 */
ncwrap_error
input_window_init(input_window_t iw, int x, int y, int width,
                  const char *title);

/* Close input window.
 * @param iw[in,out]: the window to be closed.
 * @return: error code.
 */
ncwrap_error
input_window_close(input_window_t iw);

/* Read string from input window.
 * @param iw[in]: the target window.
 * @param buff[out]: the buffer to store the C style string in.
 * @param buff_sz[in]: the size of the read string.
 * @return: error code.
 */
ncwrap_error
input_window_read(input_window_t iw, char *buff, size_t buff_sz);

/* Initialize scroll window with dimensions and title.
 * @param sw[out]: the initialized window.
 * @param x[in]: the horizontal position of the top-left corner.
 * @param y[in]: the vertical position of the top-left corner.
 * @param width[in]: the width of the window.
 * @param height[in]: the height of the window.
 * @param title[in]: the title of the window.
 * @return: error code.
 */
ncwrap_error
scroll_window_init(scroll_window_t sw, int x, int y, int width, int height,
                   const char *title);

/* Close scroll window.
 * @param sw[in,out]: the target window.
 * @return: error code.
 */
ncwrap_error
scroll_window_close(scroll_window_t sw);

/* Add line to the scroll window.
 * @param sw[in]: the target window.
 * @param line[in]: the line to be added to the window.
 * @return: error code.
 */
ncwrap_error
scroll_window_add_line(scroll_window_t sw, const char *line);

// option callback type
typedef void (*option_cb)(void *ctx);

/* Initialize menu window with dimensions and title.
 * @param mw[out]: the initialized window.
 * @param x[in]: the horizontal position of the top-left corner.
 * @param y[in]: the vertical position of the top-left corner.
 * @param width[in]: the width of the window.
 * @param height[in]: the height of the window.
 * @param title[in]: the title of the window.
 * @return: error code.
 */
ncwrap_error
menu_window_init(menu_window_t mw, int x, int y, int width, int height,
                 const char *title);

/* Close menu window.
 * @param mw[in,out]: the target window.
 * @return: error code.
 */
ncwrap_error
menu_window_close(menu_window_t mw);

/* Start event loop in the menu window.
 * @param mw[in]: the target window.
 * @return: error code.
 */
ncwrap_error
menu_window_start(menu_window_t mw);

/* Add option to the menu window.
 * @param mw[in]: the target window.
 * @param label[in]: the name of the new option.
 * @param cb[in]: the callback of the new option.
 * @param ctx[in]: the context of the callback.
 * @return: error code.
 */
ncwrap_error
menu_window_add_option(menu_window_t mw, const char *label, option_cb cb,
                       void *ctx);

/* Delete option from the menu window.
 * @param mw[in]: the target window.
 * @param label[in]: the name of the option.
 * @return: error code.
 */
ncwrap_error
menu_window_delete_option(menu_window_t mw, const char *label);

#endif // NCWRAP_H_HEADER_GUARD
