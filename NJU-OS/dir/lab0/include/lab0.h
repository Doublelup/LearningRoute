#ifndef LAB0_H
#define LAB0_H
#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>
#include <keyboard.h>
#include<graph.h>

// extern struct screen screen;

extern void graph_init();
extern void background_init();
extern void background();
extern void ball_init();
extern void ball(keycode code);

extern void keyboard_init();
extern keycode read_keyboard();

#endif