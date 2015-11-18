#include "window.h"
#include <log.h>

enum {
    ERR_WINCREATE = 10,
};

void window_set(window_t *win, WINDOW *wptr, ushort h, ushort w, ushort y, ushort x) {
    if(wptr != NULL)
        win->win = wptr;
    win->h = h;
    win->w = w;
    win->y = y;
    win->x = x;
}

int window_create(window_t *win) {
    win->win = newwin(win->h, win->w, win->y, win->x);
    if(win->win == NULL) {
        lc_error("ERROR - window_create(): newwin() returned NULL");
        window_errno = ERR_WINCREATE;
        return ERR_WINCREATE;
    }

    return 0;
}

int window_delete(window_t *win) {
    if((window_errno = delwin(win->win)) != OK){
        lc_error("ERROR - window_delete(): delwin() failed with code %i)", window_errno);
        return window_errno;
    }
    return 0;
}

int window_clear(window_t* win){
    if((window_errno = wclear(win->win)) != OK){
        lc_error("ERROR - window_clear(): wclear() failed with code %i)", window_errno);
        return window_errno;
    }
    return 0;
}

int window_drawborder(window_t* win){
    if((window_errno = box(win->win, 0, 0)) != OK){
        lc_error("ERROR - window_drawborder(): box() failed with code %i)", window_errno);
        return window_errno;
    }
    return 0;
}

int window_refresh(window_t *win) {
    if((window_errno = wrefresh(win->win)) != OK){
        lc_error("ERROR - window_refresh(): wrefresh() failed with code %i)", window_errno);
        return window_errno;
    }
    return 0;
}
