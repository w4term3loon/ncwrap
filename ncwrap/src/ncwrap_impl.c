#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>

#include "ncurses.h"
#include "ncwrap_impl.h"

// --------------------------------------------------
//    TODO: scroll window line memory management
//    TODO: possible bugs with unrecognised characters (TAB)
//    TODO: solve scoping when cursor is on 0
//    TODO: indicate nothing happened
//    TODO: enable resize of window
//    TODO: add box border art
//    TODO: introduce thread safety
//    TODO: debug window for internal error messages
// --------------------------------------------------

void ncurses_init()
{
    (void)initscr();
    (void)nonl();
    (void)cbreak();
    (void)noecho();
    (void)curs_set(0);
}

void ncurses_close()
{
    (void)endwin();
}

input_window_t* input_window_init(int x, int y, int width, const char* title)
{
    input_window_t* input_window =
        (input_window_t*)malloc(sizeof(input_window_t) + sizeof((*title) + 1));
    if (input_window == NULL)
    {
        fprintf(stderr, "ERROR: input window init failed.\n");
        return (input_window_t*)NULL;
    }
    
    input_window->title = (char*)(input_window + sizeof(input_window));
    strcpy(input_window->title, title);
    input_window->window = newwin(3, width, y, x);
    input_window->width = width;
    
    box(input_window->window, 0, 0);
    mvwprintw(input_window->window, 0, 1, " %s ", title);
    wrefresh(input_window->window);
    
    return input_window;
}

void input_window_close(input_window_t* input_window)
{
    delwin(input_window->window);
    free(input_window);
    input_window = NULL;
}

void delete(char* buff, size_t buff_siz, int idx)
{
    for (size_t i = 0; i < buff_siz; ++i)
    {
        if (i >= idx)
        { buff[i] = buff[i + 1]; }
    }
}

void insert(char* buff, size_t buff_siz, int idx, char ch)
{
   	for (size_t i = buff_siz - 1; i > idx; --i)
   	{
       	if (i > idx)
       	{ buff[i] = buff[i - 1]; }
   	}
	    
    buff[idx] = ch;
}

// possible bugs with unrecognised characters
int input_window_read(input_window_t* input_window, char *buff, size_t buff_siz)
{
    wmove(input_window->window, 1, 1);
    curs_set(2);
    wrefresh(input_window->window);
    
    int scope = 0;
    int next, cursor = 0;
    int width = input_window->width;
    char display[width];
    int capture = 0;
    for (size_t i = 0; i < buff_siz - 1; ++i)
    {
        capture = 0;
        next = wgetch(input_window->window);
        if (31 < next && next < 128) { ++capture; }
        switch (next)
        {
            case 127: // backspace
            case KEY_DC:
            case KEY_BACKSPACE:
                if (scope != 0 || cursor != 0) {
                    delete(buff, buff_siz, scope + cursor - 1);
                    if (scope != 0 && (cursor + scope == i || cursor <= width))
                    { scope -= 1; }
                    else { cursor -= 1; }
                    i -= 2;
                }
                else { --i; } // indicate nothing happened
                break;
                
            case '\n': // return
            case '\r':
            case KEY_ENTER:
                buff[i] = '\0';
                curs_set(0);
                clear_window_content(input_window->window, input_window->title);
                return i + 1;
                break;
            
            case 27: // escape
                wgetch(input_window->window);
                switch(wgetch(input_window->window))
                {
                    case 'D': // left arrow
                    case KEY_LEFT:
                        if (cursor != 0 )
                        { cursor -= 1; }
                        else
                        {
                            if (scope != 0)
                            { scope -= 1; }
                        }
                        break;
                    
                    case 'C': // right arrow
                    case KEY_RIGHT:
                        if (cursor != width - 3)
                        {
                            if (cursor < i)
                            { cursor += 1; }
                        }
                        else if (cursor + scope != i)
                        { scope += 1; }
                        break;
                        
                    case 'A': // up arrow
                    case 'B': // down arrow
                    case KEY_UP:
                    case KEY_DOWN:
                        break;
                        
                    default:;
                }
                --i;
                break;
                
            default:
                if (capture)
                {
                	insert(buff, buff_siz, scope + cursor, (char)next);
                	if (width - 3 == cursor)
                	{ scope += 1; }
                	else { cursor += 1; }

                } else break;
        }
        
		buff[i + 1] = '\0';
        clear_window_content(input_window->window, input_window->title);
        
        for (int j = 0; j < width - 1; ++j)
        {
            display[j] = buff[scope + j];
			if (j == input_window->width - 2)
			{ display[j] = '\0'; };
        }
    
        mvwprintw(input_window->window, 1, 1, "%s", display);

		// left indicator
		if (scope != 0) 
		{ mvwprintw(input_window->window, 1, 0, "<"); }
		
		// right indicator
		if (i - scope + 1 >= width - 2) 
		{ mvwprintw(input_window->window, 1, width - 1, ">"); }

        wmove(input_window->window, 1, 1 + cursor);
    }
    
    curs_set(0);
	clear_window_content(input_window->window, input_window->title);

    return buff_siz;
}

scroll_window_t* scroll_window_init(int x, int y, int width, int height, const char* title)
{
    scroll_window_t* scroll_window =
        (scroll_window_t*)malloc(sizeof(scroll_window_t) + sizeof((*title) + 1));
    if (scroll_window == NULL)
    {
        fprintf(stderr, "ERROR: scroll window init failed.\n");
        return (scroll_window_t*)NULL;
    }
    
    scroll_window->title = (char*)(scroll_window + sizeof(scroll_window));
    strcpy(scroll_window->title, title);
    scroll_window->window = newwin(height, width, y, x);
    scroll_window->width = width;
    scroll_window->height = height;
    
    box(scroll_window->window, 0, 0);
    mvwprintw(scroll_window->window, 0, 1, " %s ", title);
    wrefresh(scroll_window->window);
    
    scrollok(scroll_window->window, TRUE);
    
    return scroll_window;
}

void scroll_window_close(scroll_window_t* scroll_window)
{
    delwin(scroll_window->window);
    free((void*)scroll_window);
    scroll_window = NULL;
}

void scroll_window_newline(scroll_window_t* scroll_window, const char* line)
{
    scroll(scroll_window->window);
    
    wmove(scroll_window->window, scroll_window->height - 2, 1);
    wclrtoeol(scroll_window->window);
    
    mvwprintw(scroll_window->window, scroll_window->height - 2, 1, "%s", line);
    box(scroll_window->window, 0, 0);

    mvwprintw(scroll_window->window, 0, 1, " %s ", scroll_window->title);
    wrefresh(scroll_window->window);
}

//---------
// helpers |
//---------

void clear_window_content(WINDOW* window, char* title)
{
    werase(window);
    box(window, 0, 0);
    mvwprintw(window, 0, 1, " %s ", title);
}




