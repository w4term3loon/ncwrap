#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

#include "ncurses.h"
#include "ncwrap_impl.h"

// --------------------------------------------------
//    TODO: scroll window line memory management
//    TODO: possible bugs with unrecognised characters (TAB)
//    TODO: solve scoping when cursor is on 0
//    TODO: indicate nothing happened
//    TODO: enable resize of window
//    TODO: add box border art
//    TODO: introduce thread safety ??
//    TODO: debug window for internal error messages
//    TODO: handle multiple menu items with the same name (delete)
//    TODO: terminal window
// --------------------------------------------------

void menu_window_update(menu_window_t* menu_window, int highlight);

void ncwrap_error(const char *ctx)
{
	(void)fprintf(stderr, "ERROR: ncwrap failed during %s.\n", ctx);
}

void ncwrap_init()
{
    (void)initscr();
    (void)nonl();
    (void)cbreak();
    (void)noecho();
    (void)curs_set(0);
}

void ncwrap_close()
{
    (void)endwin();
}
 
input_window_t* input_window_init(int x, int y, int width, const char* title)
{
    input_window_t* input_window =
        (input_window_t*)malloc(sizeof(input_window_t) + strlen(title) + 1);
    if (input_window == NULL)
    {
		ncwrap_error("input window init");
        return (input_window_t*)NULL;
    }
    
    input_window->title = (char*)(input_window + 1);
    (void)strncpy(input_window->title, title, strlen(title) + 1);
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
    free((void *)input_window);
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
        (scroll_window_t*)malloc(sizeof(scroll_window_t) + strlen(title) + 1);
    if (scroll_window == NULL)
    {
		ncwrap_error("scroll window init");
        return (scroll_window_t*)NULL;
    }
    
    scroll_window->title = (char*)(scroll_window + 1);
    (void)strncpy(scroll_window->title, title, strlen(title) + 1);
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
    free((void *)scroll_window);
    scroll_window = NULL;
}

void scroll_window_add_line(scroll_window_t* scroll_window, const char* line)
{
    scroll(scroll_window->window);
    
    wmove(scroll_window->window, scroll_window->height - 2, 1);
    wclrtoeol(scroll_window->window);
    
    mvwprintw(scroll_window->window, scroll_window->height - 2, 1, "%s", line);
    box(scroll_window->window, 0, 0);

    mvwprintw(scroll_window->window, 0, 1, " %s ", scroll_window->title);
    wrefresh(scroll_window->window);
}

menu_window_t* menu_window_init(int x, int y, int width, int height, const char* title)
{
	menu_window_t *menu_window =
		(menu_window_t *)malloc(sizeof(menu_window_t) + strlen(title) + 1);
	if (menu_window == NULL)
	{
		ncwrap_error("menu window init");
        return (menu_window_t *)NULL;
	}
    
	menu_window->title = (char *)(menu_window + 1);
    (void)strncpy(menu_window->title, title, strlen(title) + 1);
    menu_window->window = newwin(height, width, y, x);
    menu_window->width = width;
    menu_window->height = height;
    menu_window->options = (option_t *)NULL;
	menu_window->options_num = 0;

    box(menu_window->window, 0, 0);
    mvwprintw(menu_window->window, 0, 1, " %s ", title);
    wrefresh(menu_window->window);
    
    return menu_window;
}

void menu_window_close(menu_window_t *menu_window)
{
    delwin(menu_window->window);
	for (int i = 0; i < menu_window->options_num; ++i)
	{
		free((void *)(menu_window->options + i)->name);
	}
	free((void *)menu_window->options);
    free((void *)menu_window);
    menu_window = NULL;
}

void menu_window_add_option(menu_window_t *menu_window, const char *name, void (*cb)(void *), void *ctx)
{
	void *new_options = realloc(menu_window->options, (menu_window->options_num + 1) * sizeof(option_t));
	if (new_options == NULL)
	{
		ncwrap_error("menu window add option");
        return;
	}
	menu_window->options = new_options;
	
	option_t *new_option = (menu_window->options + menu_window->options_num);
	new_option->name = (char *)malloc(strlen(name) + 1);
	if (new_option->name == NULL)
	{
		ncwrap_error("menu window add option");
        return;
	}

	(void)strncpy(new_option->name, name, strlen(name) + 1);
	new_option->cb = cb;
	new_option->ctx = ctx;
	
	menu_window->options_num += 1;
	menu_window_update(menu_window, 0);
}

void squash(menu_window_t *menu_window, int option_offset)
{
	// free dynamically allocated name
	free((void *)(menu_window->options + option_offset)->name);
	if (option_offset == menu_window->options_num - 1)
	{
		return; // realloc will take care of the last element
	}
	else
	{
		// shift all elements back one place, starting after the offset
		for (int i = option_offset; i < menu_window->options_num - 1; ++i)
		{
			(menu_window->options + i)->name = (menu_window->options + i + 1)->name;
			(menu_window->options + i)->cb = (menu_window->options + i + 1)->cb;
			(menu_window->options + i)->ctx = (menu_window->options + i + 1)->ctx;
		}
	}
}

void menu_window_delete_option(menu_window_t *menu_window, const char *name)
{
	for (int i = 0; i < menu_window->options_num; ++i)
	{
		if (strcmp((menu_window->options + i)->name, name) == 0)
		{
			squash(menu_window, i);
			
			void *new_options = realloc(menu_window->options, (menu_window->options_num - 1) * sizeof(option_t));
			if (new_options == NULL)
			{
				ncwrap_error("menu window delete option");
				return;
			}
			menu_window->options = new_options;

			--menu_window->options_num;
			menu_window_update(menu_window, 0);
			break;
		}
	}
}

void menu_window_update(menu_window_t* menu_window, int highlight) {

	clear_window_content(menu_window->window, menu_window->title);
    for (int i = 0; i < menu_window->options_num; ++i)
	{
        if (highlight == i) { wattron(menu_window->window, A_REVERSE); }
		mvwprintw(menu_window->window, i + 1, 1, "%s", (menu_window->options + i)->name);
        if (highlight == i) { wattroff(menu_window->window, A_REVERSE); }
	}

	// wrefresh(menu_window->window);

	/*
    std::string temp_line;
    int net_width = menu_window->width - 2;

    for(size_t i = 0; i < menu_window->items.size(); ++i) {

        if (net_width < 4) {

            for (size_t j = 0; j < (size_t)net_width; ++j) {
                temp_line.push_back('.');
            }

        } else if ((size_t)net_width < menu_window->items[i].label.size()){

            for (size_t j = 0; j < (size_t)net_width - 3; ++j) {
                temp_line.push_back(menu_window->items[i].label[j]);
            }

            temp_line = temp_line + "...";

        } else {

            temp_line = menu_window->items[i].label;

            size_t padding = (size_t)net_width - menu_window->items[i].label.size();
            for (size_t j = 0; j < padding; ++j) {
                temp_line.push_back(' ');
            }
        }

        if ((size_t)highlight == i) { wattron(menu_window->window, A_REVERSE); }
        mvwprintw(menu_window->window, i + 1, 1, "%s", temp_line.c_str());
        if ((size_t)highlight == i) { wattroff(menu_window->window, A_REVERSE); }

        temp_line.clear();
    }
	*/
}

void menu_window_start(menu_window_t *menu_window) {
    int ch;
    int highlight = 0;
    while(true)
	{
        while(true) {
            if (highlight >= menu_window->options_num) {
                --highlight;
            } else {
                break;
            }
        }

        menu_window_update(menu_window, highlight);

        ch = wgetch(menu_window->window);
        switch(ch)
        {
        case 107: // k
            if (highlight != 0)
                --highlight;
            break;

        case 106: // j
            if (highlight != menu_window->options_num - 1)
                ++highlight;
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
            (menu_window->options + highlight)->cb((menu_window->options + highlight)->ctx);
            break;

        case 27:    //< Esc
        case 113:   //< q
            goto exit;

        default:
            ;
        }

        if (false) {
        exit:
            break;
        }
	}
}

void clear_window_content(WINDOW* window, char* title)
{
    werase(window);
    box(window, 0, 0);
    mvwprintw(window, 0, 1, " %s ", title);
}

