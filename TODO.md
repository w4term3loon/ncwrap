## TODO list
============
### Bugs:
* opening multiple popups cause segfault when enter is hit.
* if input window output is not set segfault

### General improvements
* PURGE THE FOCUS SYSTEM (~rework)
* set visibility on functions
* https://www.man7.org/linux/man-pages/man3/curs_inopts.3x.html
* comment for all magic constants
* only refresh windows that had changed (handler called on)
* support for popup windows proper lifecycle management
* introduce thread safety ??

### Visual updates
* indicate nothing happened input window (retval: maybe let application handle)
* indicate nothing happened menu window (retval: maybe let application handle)
* enable resize of window
* add box border art

### Ideas
* err code interpreter fuction on interface
* logging with dle or syslog or stderr
* terminal window
* debug window
* game window
* custom window
* window editor window
* save log from scroll window
* plot window
* UI windows that can display telemetry data
* login window
* header menus for windows
* macro keys for windows that call callbacks
* err message box


