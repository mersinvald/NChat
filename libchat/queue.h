#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdlib.h>

#define LC_QUEUE_INITIALIZER(ssize, mtxptr) {NULL, 0, ssize, mtxptr}

/* list node */
struct lc_qnode_s {
    void* var;
    struct lc_qnode_s *next;
};

/* queue header */
struct lc_queue_s{
    struct lc_qnode_s* top;  /* first node */
    ushort lenght;        /* queue length */
    size_t ssize;         /* sizeof(element_type) */

    pthread_mutex_t* mtx; /* mutex for purpose of multithreading usage */
};

typedef struct lc_queue_s lc_queue_t;
typedef struct lc_qnode_s lc_qnode_t;

extern int         lc_queue_add(lc_queue_t* q, void* var);
/* queue an element
 * @param
 * queue* q  - queue header
 * void* var - void pointer to element
 *
 * @return
 *  0 if OK
 * -1 if error
 */

extern void*       lc_queue_pop(lc_queue_t* q);
/* extract the first element
 * @param
 * queue* q  - queue header
 *
 * @return
 * void* ptr on the first element if success (MUST BE FREED)
 * NULL if error
 */

extern lc_qnode_t* lc_queue_back(lc_queue_t* q);
/* get pointer to the last node
 * @param
 * queue* q  - queue header
 *
 * @return
 * qnode* prt to the last node if success
 * NULL if error
 */

#endif // QUEUE_H
