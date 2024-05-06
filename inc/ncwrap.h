#ifndef NCWRAP_H_HEADER_GUARD
#define NCWRAP_H_HEADER_GUARD

#include <stddef.h>
#include <stdint.h>

#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

#undef ERR
#define ERR (-1)

#undef OK
#define OK (0)

typedef enum {
  NCW_OK = 0,              // OK
  NCW_NCURSES_FAIL,        // ncurses library ran into error (?)
  NCW_INVALID_PARAM,       // invalid parameter for function
  NCW_INSUFFICIENT_MEMORY, // application ran out of memory
} ncw_err;

// window handler type
typedef struct window_handle *window_handle_t;

// input window type
typedef struct input_window *input_window_t;

// scroll window type
typedef struct scroll_window *scroll_window_t;

// menu window type
typedef struct menu_window *menu_window_t;

/* Initialize the library.
 * Always call before the first window call.
 */
ncw_err
ncw_init(void);

/* Deinitialize the library.
 * Always call after the last window call.
 */
ncw_err
ncw_close(void);

/* Get input from user.
 * @return the ascii decimal of the inputted character.
 */
int
ncw_getch(void);

/* Step the focus handle to the next window with handler.
 * @param focus[in,out]: the focus acquired by the ncw_focus_get function.
 */
void
ncw_focus_step(void);

/* Update all windows on the screen.
 */
void
ncw_update(void);

/* Handle an event recieved from ncw_getch by the focused window.
 * @param event[in]: the event that from ncw_getch.
 */
void
ncw_event_handler(int event);

/* Initialize input window with dimensions and title.
 * @param iw[out]: the initialized window.
 * @param x[in]: the horizontal position of the top-left corner.
 * @param y[in]: the vertical position of the top-left corner.
 * @param width[in]: the width of the window.
 * @param title[in]: the title of the window.
 * @param is_popup[in]: is the window a popup. (close after return)
 * @return: error code.
 */
ncw_err
ncw_input_window_init(input_window_t *iw, int x, int y, int width,
                      const char *title, char is_popup);

/* Close input window and set the pointer to NULL.
 * @param iw[in,out]: the window to be closed.
 * @return: error code.
 */
ncw_err
ncw_input_window_close(input_window_t *iw);

/* Input window output callback type. */
typedef void (*output_cb)(char *buf, size_t bufsz, void *ctx);

/* Set output for input window event.
 * @param iw[in]: the target window.
 * @param output_cb[in]: the callback which is called when line is read.
 * @param ctx[in]: the context for the callback.
 * @return: error code.
 */
ncw_err
ncw_input_window_set_output(input_window_t iw, output_cb cb, void *ctx);

/* Initialize scroll window with dimensions and title.
 * @param sw[out]: the initialized window.
 * @param x[in]: the horizontal position of the top-left corner.
 * @param y[in]: the vertical position of the top-left corner.
 * @param width[in]: the width of the window.
 * @param height[in]: the height of the window.
 * @param title[in]: the title of the window.
 * @return: error code.
 */
ncw_err
ncw_scroll_window_init(scroll_window_t *sw, int x, int y, int width, int height,
                       const char *title);

/* Close scroll window and set the pointer to NULL.
 * @param sw[in,out]: the target window.
 * @return: error code.
 */
ncw_err
ncw_scroll_window_close(scroll_window_t *sw);

/* Add line to the scroll window.
 * @param sw[in]: the target window.
 * @param line[in]: the line to be added to the window.
 * @return: error code.
 */
ncw_err
ncw_scroll_window_add_line(scroll_window_t sw, const char *line);

/* Menu window option callback type. */
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
ncw_err
ncw_menu_window_init(menu_window_t *mw, int x, int y, int width, int height,
                     const char *title);

/* Close menu window and set the pointer to NULL.
 * @param mw[in,out]: the target window.
 * @return: error code.
 */
ncw_err
ncw_menu_window_close(menu_window_t *mw);

/* Add option to the menu window.
 * @param mw[in]: the target window.
 * @param label[in]: the name of the new option.
 * @param cb[in]: the callback of the new option.
 * @param ctx[in]: the context of the callback.
 * @return: error code.
 */
ncw_err
ncw_menu_window_add_option(menu_window_t mw, const char *label, option_cb cb,
                           void *ctx);

/* Delete option from the menu window.
 * @param mw[in]: the target window.
 * @param label[in]: the name of the option.
 * @return: error code.
 */
ncw_err
ncw_menu_window_delete_option(menu_window_t mw, const char *label);

#endif // NCWRAP_H_HEADER_GUARD
