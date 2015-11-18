#ifndef INTERFACE_H
#define INTERFACE_H
#include <queue.h>

#define ERR_CURSES 10
#define ERR_INPUT  11

struct interface_thread_data_s {
    lc_queue_t *inqueue;
    lc_queue_t *outqueue;
};
typedef struct interface_thread_data_s interface_tdata_t;

void* interface(void* arg);
#endif // INTERFACE_H
