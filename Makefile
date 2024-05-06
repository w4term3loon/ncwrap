CFLAGS=-fPIC -Wall -pedantic -I./inc
IFACE=inc/ncwrap.h
LD=-lncurses
OUT=libncw
CC=gcc

.PHONY: all clean rebuild
rebuild: clean all
static: $(OUT).a
shared: $(OUT).so
all: static

app: $(OUT).a
	$(CC) -lncurses $(CFLAGS) app.c $< -o app

$(OUT).so: $(OUT).o
	$(CC) $(LD) -$@ $< -o $@

$(OUT).a: $(OUT).o
	ar rcs $@ $<

$(OUT).o: src/ncwrap_impl.c $(IFACE)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f ./$(OUT).[oa]
