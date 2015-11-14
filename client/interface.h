#ifndef INTERFACE_H
#define INTERFACE_H
#include <msgqueue.h>

typedef struct interface_thread_data_s {
    msgqueue *inqueue;
    msgqueue *outqueue;
} interface_tdata;

void* interface(void* arg);

int init();


#endif // INTERFACE_H

