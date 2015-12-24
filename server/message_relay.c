#include "message_relay.h"

#include <sys/ioctl.h>
#include <signal.h>

#include <types.h>
#include <log.h>
#include <non_block_io.h>

#include <libexplain/socket.h>
#include <errno.h>

#include <memory.h>
#include <unistd.h>

volatile int  relay_done = 0;
volatile int  lastclindex = 0;
struct relay_s* fds;

int add_client(struct relay_s *fds, int fd){
    int* temp = realloc(fds->clientsfd, (++fds->count) * sizeof(int));
    if(temp == NULL) return -1;

    fds->clientsfd = temp;
    fds->clientsfd[fds->count-1] = fd;
    return 0;
}

int del_client(struct relay_s *fds, int index){
    shutdown(fds->clientsfd[index], SHUT_RDWR);
    close(fds->clientsfd[index]);
    if(fds->count > 1) {
        fds->clientsfd[index] = fds->clientsfd[fds->count-1];
        fds->clientsfd = realloc(fds->clientsfd, (fds->count-1) * sizeof(int));
        fds->count--;
    } else {
        free(fds->clientsfd);
        fds->count = 0;
    }
    return 0;
}

void broadcast(struct relay_s *fds,lc_message_t* msg, int size){
    int i, fd;
    for(i = 0; i < fds->count; i++){
        lastclindex = i;
        fd = fds->clientsfd[i];
        if(lc_send_non_block(fd, msg, size, 0) <= 0){
            lc_error("Error occured while broadcasting. Retrying");
            i = lastclindex - 1;
        }
     }
}

void relay_sigpipe_handler(int signum){
    if(signum == SIGPIPE){
        del_client(fds, lastclindex);
        lc_error("Client's #%i connection died, deleting.", lastclindex);
    }
}

void relay_term_handler(int signum){
    char sig[16];
    switch(signum){
    case SIGINT:  strcpy(sig, "SIGINT");  break;
    case SIGTERM: strcpy(sig, "SIGTERM"); break;
    case SIGKILL: strcpy(sig, "SIGKILL"); break;
    default:  sprintf(sig, "SIGNAL %i", signum);
    }
    lc_log_v(1, "relay: Got %s, shutting down.", sig);
    relay_done = 1;
}

void* relay_thread(void* arg){
    /* Making relay react on SIGTERM and SIGKILL */
    struct sigaction termaction;
    memset(&termaction, 0, sizeof(struct sigaction));
    termaction.sa_handler = relay_term_handler;
    sigaction(SIGTERM, &termaction, NULL);
    sigaction(SIGKILL, &termaction, NULL);
    sigaction(SIGINT, &termaction, NULL);

    /* Sigpipe handler to disable broken connections */
    struct sigaction pipeaction;
    memset(&pipeaction, 0, sizeof(struct sigaction));
    pipeaction.sa_handler = relay_sigpipe_handler;
    sigaction(SIGPIPE, &pipeaction, NULL);

    /* Get more useful pointers from arg */
    lc_queue_t* fdq = (lc_queue_t*) arg;
    pthread_mutex_t* mtx = fdq->mtx;

    /* Initializing relay struct */
    fds = malloc(sizeof(fds));
    fds->clientsfd = NULL;
    fds->count = 0;

    int n, i, fd, *ptr;
    lc_message_t msg;
    size_t msg_size;
    memset(&msg, '\0', sizeof(lc_message_t));

    /* relay loop */
    while(!relay_done){
        /* Receiving new client's fd from listener */
        pthread_mutex_lock(mtx);
        while(fdq->lenght > 0){
            ptr = (int*) lc_queue_pop(fdq);
            if(add_client(fds, *ptr) < 0){
                lc_error("ERROR - add_client(): can't add new client, realloc() failure\nProbably low memory or corruption");
                goto exit;
            }
            free(ptr);
            lc_log_v(5, "relay: Got new client from listener");
        }
        pthread_mutex_unlock(mtx);

        for(i = 0; i < fds->count; i++){
            lastclindex = i;
            memset(&msg, '\0', sizeof(lc_message_t));
            fd = fds->clientsfd[i];
            n = lc_recv_non_block(fd, &msg, sizeof(msg), 0);
            if(n < 0){
                lc_error("ERROR - lc_recv_non_block(): returned -1");
            }
            if(n > 0){
                lc_log_v(4, "Received message from one of clients --> broadcasting...");
                msg_size = sizeof(msg.username) + strlen(msg.text) + 1;
                broadcast(fds, &msg, msg_size);
            }
        }

       usleep(10000);
    }

exit:
    lc_log_v(1, "Shutting down all connections");
    for(i = 0; i < fds->count; i++){
        shutdown(fds->clientsfd[i], SHUT_RDWR);
        close(fds->clientsfd[i]);
    }
    lc_log_v(3, "Freeing relay");
    free(fds->clientsfd);
    free(fds);
    exit(0);
}
