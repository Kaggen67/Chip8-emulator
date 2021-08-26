#!/bin/sh

gcc -Wall -g chip8_gtk_ui.c chip8_gtk_debug.c chip8.c chip8_dis.c chip8_gtk_keypad.c chip8_gtk_menu.c -o chip8 `pkg-config --libs --cflags gtk+-3.0`

# `pkg-config --libs --cflags glib-2.0` `pkg-config --libs --cflags gdk-3.0`
