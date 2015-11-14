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

volatile int lastcliindex = 0;
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
    int i, fd, n;
    for(i = 0; i < bay->count; i++){
        lastcliindex = i;
        fd = bay->clientsfd[i];
        n = lc_send_non_block(fd, msg, size, 0);
        if(n < 0){
            lc_error("ERROR - lc_send_non_block(): got -1, deleting this client");
        }
     }
}

void sigpipe_handler(int signum){
    if(signum == SIGPIPE){
        del_client(bay, lastcliindex);
        lc_error("Client's #%i connection died, deleting.", lastcliindex);
    }
}

void* bay_thread(void* arg){
    /* Making server react on SIGTERM and SIGKILL */
    struct sigaction termaction;
    memset(&termaction, 0, sizeof(struct sigaction));
    termaction.sa_handler = lc_term;
    sigaction(SIGTERM, &termaction, NULL);
    sigaction(SIGKILL, &termaction, NULL);
    sigaction(SIGINT, &termaction, NULL);

    /* Sigpipe handler to disable broken connections */
    struct sigaction pipeaction;
    memset(&pipeaction, 0, sizeof(struct sigaction));
    pipeaction.sa_handler = sigpipe_handler;
    sigaction(SIGPIPE, &pipeaction, NULL);

    queue* fdque = (queue*) arg;
    pthread_mutex_t* mtx = fdque->mtx;

    bay = malloc(sizeof(bay));
    bay->clientsfd = NULL;
    bay->count = 0;
    struct bay_s* bayptr = bay;

    int n, i, fd, *ptr;
    message chatmsg;
    memset(&chatmsg, '\0', sizeof(message));

    while(!lc_done){
        pthread_mutex_lock(mtx);
        while(fdque->lenght > 0){
            ptr = (int*) pop(fdque);
            if(add_client(bay, *ptr) < 0){
                lc_error("ERROR - add_client(): can't add new client, realloc() failure\nProbably low memory or corruption");
                goto exit;
            }
            free(ptr);
        }
        pthread_mutex_unlock(mtx);

        for(i = 0; i < bay->count; i++){
            memset(&chatmsg, '\0', sizeof(message));
            lastcliindex = i;
            fd = bay->clientsfd[i];
            n = lc_recv_non_block(fd, &chatmsg, sizeof(chatmsg), 0);
            if(n < 0){
                lc_error("ERROR - lc_recv_non_block(): returned -1");
            }
            if(n > 0){
                lc_log_v(4, "Received message from one of clients --> broadcasting...");
                broadcast(bay, &chatmsg, sizeof(chatmsg));
            }
        }

       usleep(100000);
    }

exit:
    lc_log_v(3, "Freeing bay");
    free(bay->clientsfd);
    free(bay);
    exit(0);
}
