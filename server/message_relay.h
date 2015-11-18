#ifndef MESSAGE_RELAY_H
#define MESSAGE_RELAY_H
#include "server.h"
#include "queue.h"
#include <pthread.h>
#include <types.h>

/* pthread function for message relay */
extern void* relay_thread(void* arg);

/* structure for client's fds array */
struct relay_s {
    int* clientsfd;
    ushort count;
};

#endif
