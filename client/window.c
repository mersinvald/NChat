#include "window.h"

/* @TODO Error Handling */

void window_set(window_t *win, WINDOW *wptr, int h, int w, int y, int x) {
    if(wptr != NULL)
        win->win = wptr;
    win->h = h;
    win->w = w;
    win->y = y;
    win->x = x;
}

int window_create(window_t *win) {
    win->win = newwin(win->h, win->w, win->y, win->x);
    if(win->win == NULL) return -1;

    return 0;
}

int window_delete(window_t *win) {
    delwin(win->win);
    return 0;
}

int window_clear(window_t* win){
    wclear(win->win);
}

int window_drawborder(window_t* win){
    box(win->win, 0, 0);
}

int window_refresh(window_t *win) {
    wrefresh(win->win);
    return 0;
}



