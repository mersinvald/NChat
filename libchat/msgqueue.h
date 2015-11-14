#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include "types.h"

struct lc_mqnode_s {
    message msg;
    struct lc_mqnode_s *next;
};

struct lc_msgqueue_s{
    struct lc_mqnode_s* top;
    int lenght;

    pthread_mutex_t mtx;
};

typedef struct lc_msgqueue_s msgqueue;
typedef struct lc_mqnode_s mqnode;

extern int      lc_msgque_queue(msgqueue* queue, message* msg);
extern message* lc_msgque_pop(msgqueue* queue);

extern mqnode* lc_msgque_back(msgqueue* queue);

#endif
