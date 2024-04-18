ncurses wrapper library
=======================

Ncwrap is an ncurses library wrapper with basic window
drawing functionalities for small projects that need a minimal terminal ui.

Depends on the ncurses development library, to install run:

    $ apt install libncurses-dev

Build the lib with:

    $ mkdir build
    $ cmake -B ./build -S ./
    $ cmake --build ./build
