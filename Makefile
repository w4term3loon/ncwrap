CFLAGS=-Wall -pedantic -I./inc
DEBUG=-g3
IFACE=inc/ncwrap.h
LD=-lncurses
OUT=libncw
CC=gcc
SRC=$(wildcard src/*.c)
OBJ=$(patsubst %.c, %.o, $(SRC))

.PHONY: all clean shared
all: libncw.a
shared: libncw.so

app: libncw.a app.c
	$(CC) $(CFLAGS) app.c $< -o app -lncurses

libncw.so: $(OBJ)
	$(CC) $(LD) -$@ $< -o $@

libncw.a: $(OBJ)
	ar rcs $@ $(OBJ)

$(OBJ): src/%.o: src/%.c $(IFACE)
	$(CC) -c -fPIC $(CFLAGS) $< -o $@

clean:
	rm -f ./$(OUT).[oa]
	rm -f ./src/*.o
