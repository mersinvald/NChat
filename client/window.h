#ifndef WINDOW_H
#define WINDOW_H
#include <ncurses.h>

struct window_s {
    WINDOW* win;
    int h, w, y, x;
};

typedef struct window_s window_t;

extern void window_set(window_t* win, WINDOW* wptr, int h, int w, int y, int x);
extern int  window_create(window_t* win);
extern int  window_delete(window_t *win);

extern int  window_clear(window_t* win);
extern int  window_drawborder(window_t* win);
extern int  window_refresh(window_t* win);

#endif // WINDOW_H


