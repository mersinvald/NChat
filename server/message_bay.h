#ifndef MESSAGE_BAY_H
#define MESSAGE_BAY_H
#include "server.h"
#include <pthread.h>
#include <types.h>

extern void* bay_thread(void* arg);  // when new socket created to here is sent it's socket's fd

struct bay_s {
    int* clientsfd;
    int count;
};

struct bay_arg {
    int* newclientfd;
    pthread_mutex_t* mtx;
};

typedef struct bay_arg bay_arg;

#endif // MESSAGE_BAY_H

