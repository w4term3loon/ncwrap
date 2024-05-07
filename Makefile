CFLAGS=-Wall -pedantic -I./inc
DEBUG=-g3
IFACE=inc/ncwrap.h
LD=-lncurses
OUT=libncw
CC=gcc

.PHONY: all clean shared
all: libncw.a
shared: libncw.so

debug: $(IFACE) src/ncwrap_impl.c app.c
	$(CC) $(DEBUG) $(CFLAGS) $^ -o $@ -lncurses

app: libncw.o app.c
	$(CC) $(CFLAGS) app.c $< -o app -lncurses

libncw.so: libncw.so: libncw.o
	$(CC) $(LD) -$@ $< -o $@

libncw.a: libncw.a: libncw.o
	ar rcs $@ $<

libncw.o: src/ncwrap_impl.c $(IFACE)
	$(CC) -c -fPIC $(CFLAGS) $< -o $@

clean:
	rm -f ./$(OUT).[oa]
