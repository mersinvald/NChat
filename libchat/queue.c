#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

int lc_queue_add(lc_queue_t* q, void* var){
    if(q == NULL) return -1;
    if(q->top == NULL){ /* КОСТЫЛИ, переделать надо */
        q->top      = malloc(sizeof(lc_qnode_t));
        q->top->next = NULL;
        q->top->var = malloc(q->ssize);
        memcpy(q->top->var, var, q->ssize);
    } else {
        lc_qnode_t* b = back(q);                 /* находим последнюю ноду */
        b->next = malloc(sizeof(lc_qnode_t));    /* резервируем память под еще одну ноду в конце */
        b->next->next = NULL;               /* принудительно зануляем указатель */
        b->next->var = malloc(q->ssize);    /* резервируем память под новый элеиент в соответствии с указанным заранее ssize */
        memcpy(b->next->var, var, q->ssize);/* копируем память из переданного указателя в новый */
    }
    q->lenght++; /* увеличиваем длину на 1 */
    return 0;
}

void* pop(lc_queue_t* q){
    if(q == NULL) return NULL;

    void* var = q->top->var;       /* этот указатель будем возвращать */

    lc_qnode_t* top = q->top;           /* запоминаем перый элемент */
    q->top = q->top->next;         /* делаем второй элемент первым */
    q->lenght--;                   /* декрементируем длину очереди */

    free(top);                     /* освобождаем бывший первый элемент,
                                      при этом значение, хранимое в нем, не затронентся,
                                      так как под него память выделялась отдельно */
    return var;                    /* возвращаем значение первого элемента.
                                      ПАМЯТЬ НАДО ОСВОБОДИТЬ ВРУЧНУЮ ПРИ ПОЛУЧЕНИИ УКАЗАТЕЛЯ */
}

lc_qnode_t* back(lc_queue_t* q){ /* тут все ясно */
    if(q == NULL) return NULL;

    lc_qnode_t* node = q->top;
    while(node->next != NULL){
        node = node->next;
    }
    return node;
}
