#include "message_bay.h"

#include <sys/ioctl.h>
#include <signal.h>

#include <types.h>
#include <log.h>
#include <non_block_io.h>
#include <sig_handler.h>
#include <ancillary.h>

#include <libexplain/socket.h>
#include <errno.h>

#include <memory.h>
#include <unistd.h>

volatile int  bay_done = 0;
volatile int  lastclindex = 0;
struct bay_s* bay;

int add_client(struct bay_s *bay, int fd){
    int* temp = realloc(bay->clientsfd, (++bay->count) * sizeof(int));
    if(temp == NULL) return -1;

    bay->clientsfd = temp;
    bay->clientsfd[bay->count-1] = fd;
    return 0;
}

int del_client(struct bay_s *bay, int index){
    shutdown(bay->clientsfd[index], SHUT_RDWR);
    close(bay->clientsfd[index]);
    if(bay->count > 1) {
        bay->clientsfd[index] = bay->clientsfd[bay->count-1];
        bay->clientsfd = realloc(bay->clientsfd, (bay->count-1) * sizeof(int));
        bay->count--;
    } else {
        free(bay->clientsfd);
        bay->count = 0;
    }
    return 0;
}

void broadcast(struct bay_s *bay, message* msg, int size){
    int i, fd;
    for(i = 0; i < bay->count; i++){
        lastclindex = i;
        fd = bay->clientsfd[i];
        if(lc_send_non_block(fd, msg, size, 0) <= 0){
            lc_error("Error occured while broadcasting. Retrying");
            i = lastclindex - 1;
        }
     }
}

void bay_sigpipe_handler(int signum){
    if(signum == SIGPIPE){
        del_client(bay, lastclindex);
        lc_error("Client's #%i connection died, deleting.", lastclindex);
    }
}

int bay_term_handler(int signum){
    char sig[16];
    switch(signum){
    case SIGINT:  strcpy(sig, "SIGINT");  break;
    case SIGTERM: strcpy(sig, "SIGTERM"); break;
    case SIGKILL: strcpy(sig, "SIGKILL"); break;
    default:  return -1;
    }
    lc_log_v(1, "Bay: Got %s, shutting down.", sig);
    bay_done = 1;
    return 0;
}

void* bay_thread(void* arg){
    /* Making server react on SIGTERM and SIGKILL */
    struct sigaction termaction;
    memset(&termaction, 0, sizeof(struct sigaction));
    termaction.sa_handler = bay_term_handler;
    sigaction(SIGTERM, &termaction, NULL);
    sigaction(SIGKILL, &termaction, NULL);
    sigaction(SIGINT, &termaction, NULL);

    /* Sigpipe handler to disable broken connections */
    struct sigaction pipeaction;
    memset(&pipeaction, 0, sizeof(struct sigaction));
    pipeaction.sa_handler = bay_sigpipe_handler;
    sigaction(SIGPIPE, &pipeaction, NULL);

    /* Get more useful pointers from arg */
    lc_queue_t* fdq = (lc_queue_t*) arg;
    pthread_mutex_t* mtx = fdq->mtx;

    /* Initializing bay struct */
    bay = malloc(sizeof(bay));
    bay->clientsfd = NULL;
    bay->count = 0;

    int n, i, fd, *ptr;
    message msg;
    memset(&msg, '\0', sizeof(message));

    /* Bay loop */
    while(!bay_done){
        /* Receiving new client's fd from listener */
        pthread_mutex_lock(mtx);
        while(fdq->lenght > 0){
            ptr = (int*) pop(fdq);
            if(add_client(bay, *ptr) < 0){
                lc_error("ERROR - add_client(): can't add new client, realloc() failure\nProbably low memory or corruption");
                goto exit;
            }
            free(ptr);
            lc_log_v(5, "Bay: Got new client from listener");
        }
        pthread_mutex_unlock(mtx);

        for(i = 0; i < bay->count; i++){
            lastclindex = i;
            memset(&msg, '\0', sizeof(message));
            fd = bay->clientsfd[i];
            n = lc_recv_non_block(fd, &msg, sizeof(msg), 0);
            if(n < 0){
                lc_error("ERROR - lc_recv_non_block(): returned -1");
            }
            if(n > 0){
                lc_log_v(4, "Received message from one of clients --> broadcasting...");
                broadcast(bay, &msg, sizeof(msg));
            }
        }

       usleep(10000);
    }

exit:
    lc_log_v(1, "Shutting down all connections");
    for(i = 0; i < bay->count; i++){
        shutdown(bay->clientsfd[i], SHUT_RDWR);
        close(bay->clientsfd[i]);
    }
    lc_log_v(3, "Freeing bay");
    free(bay->clientsfd);
    free(bay);
    exit(0);
}
