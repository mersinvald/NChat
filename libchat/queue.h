#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

/* list node */
struct qnode_s {
    void* var;
    struct qnode_s *next;
};

/* queue header */
struct queue_s{
    struct qnode_s* top;  /* first node */
    int    lenght;        /* queue length */
    size_t ssize;         /* sizeof(element_type) */

    pthread_mutex_t* mtx; /* mutex for purpose of multithreading usage */
};

typedef struct queue_s queue;
typedef struct qnode_s qnode;



extern int        add(queue* q, void* var);
/* queue an element
 * @param
 * queue* q  - queue header
 * void* var - void pointer to element
 *
 * @return
 *  0 if OK
 * -1 if error
 */

extern void*      pop(queue* q);
/* extract the first element
 * @param
 * queue* q  - queue header
 *
 * @return
 * void* ptr on the first element if success (MUST BE FREED)
 * NULL if error
 */

extern qnode*   back(queue* q);
/* get pointer to the last node
 * @param
 * queue* q  - queue header
 *
 * @return
 * qnode* prt to the last node if success
 * NULL if error
 */

#endif // QUEUE_H

