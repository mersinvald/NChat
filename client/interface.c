#include "interface.h"
#include "client.h"
#include "window.h"
#include "log.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>

volatile int resized = 0;

void resize_handler(int signum){
    resized = 1;
}

void init() {
    initscr();
    cbreak();
    curs_set(FALSE);
    refresh();
}

void reset(){
    clear();
    endwin();
    initscr();
    refresh();
}

void resize(int* W, int* H, window_t* chat, window_t* input) {
    reset();
    *W = COLS;
    *H = LINES;

    window_delete(input);
    window_delete(chat);

    int input_h = input->h;

    window_set(input, NULL, input_h, (*W), (*H)-input_h, 0);
    window_set(chat, NULL, (*H)-input_h, (*W), 0, 0);

    window_create(input);
    window_create(chat);

    window_drawborder(input);
    window_drawborder(chat);

    window_refresh(input);
    window_refresh(chat);

    resized = 0;
}

struct input_data {
    lc_queue_t* q;
    window_t*   input_win;
};

void* input_handler(void* arg){
    struct input_data* args = (struct input_data*) arg;
    lc_queue_t* q = args->q;
    window_t* win = args->input_win;
    char buffer[LC_MSG_TEXT_LEN];

    int done = 0;
    int pos = 0;
    char ch;

    while(1){
        if(mvwgetstr(win->win, 1, 1, buffer) != -1){
            pthread_mutex_lock(q->mtx);
            lc_queue_add(q, &buffer);
            memset(buffer, '\0', strlen(buffer));
            pthread_mutex_unlock(q->mtx);

            window_clear(win);
            window_drawborder(win);
            window_refresh(win);
        }
    }
}

void print_message(window_t* win, lc_message_t* msg){
    static int y = 1;
    static int x = 1;


    if(y > win->h-2){
        char space[COLS];
        memset(&space, ' ', COLS);
        space[COLS-1] = '\0';

        mvwaddstr(win->win, win->h-1, 0, space);

        wscrl(win->win, 1);
        window_drawborder(win);
        window_refresh(win);
        y--;
    }

    wattrset(win->win, A_BOLD | (!strcmp(msg->username, conf.username)) ? A_UNDERLINE : 0); /* making username bold */
    mvwprintw(win->win, y, x, "%s", msg->username);        /* print username */
    wattrset(win->win, A_NORMAL);                   /* disabling bold */
    mvwprintw(win->win, y++, x + strlen(conf.username), ": %s", msg->text);  /* print message text */
}

void* interface(void* arg){
    interface_tdata_t* data = (interface_tdata_t*) arg;
    lc_queue_t *inqueue = data->inqueue;
    lc_queue_t *outqueue = data->outqueue;

    /* Resize signal handler */
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = resize_handler;
    sigaction(SIGWINCH, &action, NULL);

    /* Setting up NCURSES */
    init();
    window_t chat, input;

    int W = COLS;
    int H = LINES;

    int input_h = 3;

    window_set(&input,    /* window_t* */
               NULL,      /* WINDOW* */
               input_h,   /* height */
               W,         /* idth */
               H-input_h, /* y pos */ /* it's like GL screen coordinates, starts from top-left */
               0);        /* x pos */
    window_set(&chat,
               NULL,
               H-input_h,
               W,
               0,
               0);
    window_create(&input);
    window_create(&chat);
    window_drawborder(&input);
    window_drawborder(&chat);
    window_refresh(&input);
    window_refresh(&chat);

    scrollok(chat.win, 1);

    /* Chat history queue */
    lc_queue_t history;
    history.lenght = 0;
    history.ssize = sizeof(lc_message_t);

    /* Setting up input thread */
    pthread_mutex_t input_mtx = PTHREAD_MUTEX_INITIALIZER;
    lc_queue_t input_queue;
    input_queue.lenght = 0;
    input_queue.mtx = &input_mtx;
    input_queue.ssize = LC_MSG_TEXT_LEN;
    struct input_data input_args = {&input_queue, &input};

    pthread_t input_thread;
    if(pthread_create(&input_thread, NULL, input_handler, &input_args)){
        lc_error("ERROR - pthread_create(): can't create input interface thread");
        goto exit_curses;
    }

    /* Interface loop */
    lc_message_t *inmsg, *outmsg;
    outmsg = malloc(sizeof(lc_message_t));
    while(1){
        /* Check if screen resized */
        if(W != COLS || H != LINES)
            resized = 1;

        /* Handle resize */
        if(resized)
            resize(&W, &H, &chat, &input);

        /* Check if there are incoming messages */
        pthread_mutex_lock(inqueue->mtx);
        if(inqueue->lenght > 0){
            inmsg = (lc_message_t*) lc_queue_pop(inqueue);
            if(inmsg == NULL)
                goto exit;
            pthread_mutex_unlock(inqueue->mtx);

            lc_queue_add(&history, inmsg);
            print_message(&chat, inmsg);
            window_refresh(&chat);

            free(inmsg);
        }
        pthread_mutex_unlock(inqueue->mtx);

        /* Check if there is user input */
        pthread_mutex_lock(&input_mtx);
        if(input_queue.lenght > 0){
            char* text = (char*) lc_queue_pop(&input_queue);
            int textlen = strlen(text);

            init_msg(outmsg);

            strcpy(outmsg->text, text);
            pthread_mutex_lock(outqueue->mtx);
            lc_queue_add(outqueue, outmsg);
            pthread_mutex_unlock(outqueue->mtx);

            memset(outmsg, '\0', textlen * sizeof(char));
            free(text);
        }
        pthread_mutex_unlock(&input_mtx);

        usleep(10000);
    }
stop_input:
    pthread_kill(input_thread, SIGKILL);
exit_curses:
    window_delete(&input);
    window_delete(&chat);
    endwin();
exit:
    pthread_exit(1);
}
