#ifndef INTERFACE_H
#define INTERFACE_H
#include <queue.h>

struct interface_thread_data_s {
    queue *inqueue;
    queue *outqueue;
};
typedef struct interface_thread_data_s interface_tdata;

void* interface(void* arg);

int init();


#endif // INTERFACE_H

