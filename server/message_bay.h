#ifndef MESSAGE_BAY_H
#define MESSAGE_BAY_H
#include "server.h"
#include "queue.h"
#include <pthread.h>
#include <types.h>

extern void* bay_thread(void* arg);  // when new socket created to here is sent it's socket's fd

struct bay_s {
    int* clientsfd;
    int count;
};

#endif // MESSAGE_BAY_H

