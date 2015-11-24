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

volatile bool resized = 0;
volatile bool resize_out = 0;

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
    window_set(chat, NULL, (*H)-input_h-1, (*W)-1, 1, 1);

    n = window_create(input);
    n = window_create(chat);

    scrollok(chat->win, true);

    n = window_drawborder(input);
    //window_drawborder(chat);

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
    char buffer[4096];
    char text[LC_MSG_TEXT_LEN];
    int len, offset;

    ushort x = 1, y = 1, i = 0;
    char ch;
    while(1){
        memset(buffer, '\0', 4096 * sizeof(char));
        while((ch = mvwgetch(win->win, y, x)) != '\n'){
            if(ch > 0){
                buffer[i++] = ch;
                if(x > win->w-3){
                    x = 0;
                    y++;
                    if(y > win->h-2) break;
                }
                x++;
            }
            usleep(1000);
        }
        y = 1; x = 1; i = 0;
        len = strlen(buffer);
        offset = 0;
        do{
            strncpy(text, buffer+offset, LC_MSG_TEXT_LEN-1);
            pthread_mutex_lock(q->mtx);
            lc_queue_add(q, &text);
            memset(text, '\0', LC_MSG_TEXT_LEN);
            pthread_mutex_unlock(q->mtx);
        } while((offset+=LC_MSG_TEXT_LEN-1) < len);
        window_clear(win);
        window_drawborder(win);
        window_refresh(win);
    }
}

void print_message(window_t* win, lc_message_t* msg){
    static ushort y = 0, x = 0;
    int i, j;

    if(resize_out == true) {
        y = 0;
        x = 0;
        resize_out = false;
    }

    if(y >= win->h) {
        y--;
        scroll(win->win);
        wrefresh(win->win);
    }

    mvwprintw(win->win, y, 0, "%s: ", msg->username);
    x += strlen(msg->username) + 2;

    for(i = 0; i < strlen(msg->text); i++){
        if(x+1 >= win->w) {y++; x=0;}
        if(y >= win->h) {
            y--;
            scroll(win->win);
            wrefresh(win->win);
        }
        mvwaddch(win->win, y, x++, msg->text[i]);
    }

    x = 0;
    y++;
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
    ushort input_h = 7;

    /* setting up windows */
    window_set(&input,    /* window_t* */
               NULL,      /* WINDOW* */
               input_h,   /* height */
               W,         /* idth */
               H-input_h, /* y pos */ /* it's like GL screen coordinates, starts from top-left */
               0);        /* x pos */
    window_set(&chat,
               NULL,
               H-input_h-1,
               W-1,
               1,
               1);

    /* wrapper-functions with some error handling */
    int n = 0;
    n = window_create(&input);
    n = window_create(&chat);
    n = window_drawborder(&input);
    //n = window_drawborder(&chat);
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
                resize_out = true;
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
            if(text != NULL) {
                ushort textlen = strlen(text);

                init_msg(outmsg);

                strcpy(outmsg->text, text);
                pthread_mutex_lock(outqueue->mtx);
                lc_queue_add(outqueue, outmsg);
                pthread_mutex_unlock(outqueue->mtx);

                memset(outmsg, '\0', textlen * sizeof(char));
                free(text);
            }
        }
        pthread_mutex_unlock(&input_mtx);

        //usleep(100);
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
