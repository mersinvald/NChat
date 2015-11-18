#include "interface.h"
#include "client.h"
#include "window.h"
#include "log.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>

#include <error.h>

volatile char resized = 0;

void resize_handler(int dummy){
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

int resize(int* W, int* H, window_t* chat, window_t* input) {
    reset();
    *W = COLS;
    *H = LINES;

    int n;

    n = window_delete(input);
    n = window_delete(chat);

    ushort input_h = input->h;

    window_set(input, NULL, input_h, (*W), (*H)-input_h, 0);
    window_set(chat, NULL, (*H)-input_h, (*W), 0, 0);

    n = window_create(input);
    n = window_create(chat);

    n = window_drawborder(input);
    window_drawborder(chat);

    n = window_refresh(input);
    n = window_refresh(chat);

    resized = 0;

    return n;
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
    static ushort y = 1;
    static ushort x = 1;


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

    int usernames = strcmp(msg->username, conf.username);

    wattrset(win->win, A_BOLD | ((usernames == 0) ? A_UNDERLINE : 0));        /* making username bold */
    mvwprintw(win->win, y, x, "%s", msg->username);                           /* print username */
    wattrset(win->win, A_NORMAL);                                             /* disabling bold */
    mvwprintw(win->win, y++, x + strlen(msg->username), ": %s", msg->text);   /* print message text */
}

volatile int interface_done = 0;

void interface_term_handler(int signum){
    char sig[16];
    switch(signum){
    case SIGINT:  strcpy(sig, "SIGINT");  break;
    case SIGTERM: strcpy(sig, "SIGTERM"); break;
    case SIGKILL: strcpy(sig, "SIGKILL"); break;
    case SIGSEGV: strcpy(sig, "SIGSEGV"); break;
    default:  sprintf(sig, "SIGNAL %i", signum);
    }
    lc_log_v(1, "Interface: Got %s, shutting down.", sig);
    interface_done = 1;
}

void* interface(void* arg){
    int* exit_code = calloc(1, sizeof(int));
    interface_tdata_t* data = (interface_tdata_t*) arg;
    lc_queue_t *inqueue = data->inqueue;
    lc_queue_t *outqueue = data->outqueue;

    /* Making interface react on SIGTERM and SIGKILL */
    struct sigaction termaction;
    memset(&termaction, 0, sizeof(struct sigaction));
    termaction.sa_handler = interface_term_handler;
    sigaction(SIGTERM, &termaction, NULL);
    sigaction(SIGKILL, &termaction, NULL);
    sigaction(SIGINT, &termaction, NULL);
    sigaction(SIGSEGV, &termaction, NULL);

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

    /* input window size (1 for input, 2 for border) */
    ushort input_h = 3;

    /* setting up windows */
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

    /* wrapper-functions with some error handling */
    int n = 0;
    n = window_create(&input);
    n = window_create(&chat);
    n = window_drawborder(&input);
    n = window_drawborder(&chat);
    n = window_refresh(&input);
    n = window_refresh(&chat);
    if(n < 0){
        lc_error("ERROR windows initialization failed with code %i", window_errno);
        *exit_code = ERR_CURSES;
        goto exit_curses;
    }

    /* Enabling curses scroll for chat window */
    scrollok(chat.win, 1);

    /* Setting up input thread */
    pthread_mutex_t input_mtx = PTHREAD_MUTEX_INITIALIZER;
    lc_queue_t input_queue    = LC_QUEUE_INITIALIZER(LC_MSG_TEXT_LEN, &input_mtx);
    struct input_data input_args = {&input_queue, &input};

    pthread_t input_thread;
    if(pthread_create(&input_thread, NULL, input_handler, &input_args)){
        lc_error("ERROR - pthread_create(): can't create input interface thread");
        *exit_code = ERR_PTHREAD;
        goto exit_curses;
    }

    /* Interface loop */
    lc_message_t *inmsg, *outmsg;
    outmsg = malloc(sizeof(lc_message_t));
    while(!interface_done){
        /* Check if screen resized */
        if(W != COLS || H != LINES)
            resized = 1;

        /* Handle resize */
        if(resized)
            if(resize(&W, &H, &chat, &input) != 0){
                lc_error("ERROR resize handling failed with code %i", window_errno);
                *exit_code = ERR_CURSES;
                goto kill_input;
            }

        /* Check if there are incoming messages */
        pthread_mutex_lock(inqueue->mtx);
        if(inqueue->lenght > 0){
            inmsg = (lc_message_t*) lc_queue_pop(inqueue);
            if(inmsg == NULL){
                lc_error("ERROR input(): input worker returned NULL");
                *exit_code = ERR_INPUT;
                goto kill_input;
            }
            pthread_mutex_unlock(inqueue->mtx);

            print_message(&chat, inmsg);
            window_refresh(&chat);

            free(inmsg);
        }
        pthread_mutex_unlock(inqueue->mtx);

        /* Check if there is user input */
        pthread_mutex_lock(&input_mtx);
        if(input_queue.lenght > 0){
            char* text = (char*) lc_queue_pop(&input_queue);
            ushort textlen = strlen(text);

            init_msg(outmsg);

            strcpy(outmsg->text, text);
            pthread_mutex_lock(outqueue->mtx);
            lc_queue_add(outqueue, outmsg);
            pthread_mutex_unlock(outqueue->mtx);

            memset(outmsg, '\0', textlen * sizeof(char));
            free(text);
        }
        pthread_mutex_unlock(&input_mtx);

        usleep(1000);
    }
kill_input:
    /* there is no sighandler in input thread, so cutting out it's fucking head */
    pthread_kill(input_thread, SIGKILL);
exit_curses:
    window_delete(&input);
    window_delete(&chat);
    clear();
    endwin();

    pthread_exit(exit_code);
}
