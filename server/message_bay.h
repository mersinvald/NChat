#ifndef MESSAGE_BAY_H
#define MESSAGE_BAY_H
#include "server.h"
#include "queue.h"
#include <pthread.h>
#include <types.h>

extern void* bay_thread(void* arg);

struct bay_s {
    int* clientsfd;
    ushort count;
};

#endif // MESSAGE_BAY_H

