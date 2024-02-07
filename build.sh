#!/bin/zsh
gcc -fPIC -c ncurses_wrapper_impl.c -I./ncurses_wrapper_impl.h -oncurses_wrapper.o
gcc -shared -olibncwrap.so ncurses_wrapper.o -lncurses -lpthread
rm ncurses_wrapper.o