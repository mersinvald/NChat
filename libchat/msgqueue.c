#ifndef MSGQUEUE_C
#define MSGQUEUE_C
#include "msgqueue.h"
#include <stdlib.h>

int lc_msgque_queue(msgqueue *queue, message *msg){
    if(queue == NULL) return -1;

    mqnode* back = lc_msgque_back(queue);
    back = calloc(sizeof(msgqueue), 0);
    back->next = NULL;

    if(memcpy(&back->msg, msg, sizeof(message)) == NULL)
        return -1;
    return 0;
}

message* lc_msgque_pop(msgqueue *queue){
    if(queue == NULL) return NULL;

    message* msg = malloc(sizeof(message));
    memcpy(msg, &(queue->top->msg), sizeof(message));

    mqnode* top = queue->top;
    queue->top = queue->top->next;
    queue->lenght--;

    free(top);
    return(msg);
}

mqnode* lc_msgque_back(msgqueue *queue){
    if(queue == NULL) return -1;

    mqnode* node = queue->top;
    while(node != NULL){
        node = node->next;
    }
    return node;
}

#endif // MSGQUEUE_C

