#ifndef WINDOW_H
#define WINDOW_H
#include <stdlib.h>
#include <ncurses.h>

struct window_s {
    WINDOW* win;
    ushort h, w, y, x;
};

volatile int window_errno;

typedef struct window_s window_t;

extern void window_set(window_t* win, WINDOW* wptr, ushort h, ushort w, ushort y, ushort x);
extern int  window_create(window_t* win);
extern int  window_delete(window_t *win);

extern int  window_clear(window_t* win);
extern int  window_drawborder(window_t* win);
extern int  window_refresh(window_t* win);

#endif // WINDOW_H
