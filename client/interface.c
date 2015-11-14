#include "interface.h"
#include "client.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void* input(void* arg){
    queue* in_q = (queue*) arg;
    char buffer[LC_MSG_TEXT_LEN];
    while(1){
        if(fgets(buffer, LC_MSG_TEXT_LEN, stdin) != NULL){
            pthread_mutex_lock(in_q->mtx);
            add(in_q, &buffer);
            memset(buffer, '\0', strlen(buffer));
            pthread_mutex_unlock(in_q->mtx);
        }
    }
}

void* interface(void* arg){
    interface_tdata* data = (interface_tdata*) arg;
    queue *inqueue = data->inqueue;
    queue *outqueue = data->outqueue;


    /* Setting up input thread */
    pthread_mutex_t input_mtx = PTHREAD_MUTEX_INITIALIZER;
    queue input_queue;
    input_queue.lenght = 0;
    input_queue.mtx = &input_mtx;
    input_queue.ssize = LC_MSG_TEXT_LEN;

    pthread_t input_thread;
    pthread_create(&input_thread, NULL, &input, &input_queue);

    /* Interface loop */
    message *inmsg, *outmsg;
    outmsg = malloc(sizeof(message));
    while(1){
        pthread_mutex_lock(inqueue->mtx);
        if(inqueue->lenght > 0){
            inmsg = (message*) pop(inqueue);
            if(inmsg == NULL)
                goto exit;
            pthread_mutex_unlock(inqueue->mtx);
            printf("%s: %s", inmsg->username, inmsg->text);
            free(inmsg);
        }
        pthread_mutex_unlock(inqueue->mtx);

        pthread_mutex_lock(&input_mtx);
        if(input_queue.lenght > 0){
            char* text = (char*) pop(&input_queue);
            int textlen = strlen(text);

            init_msg(outmsg);

            strcpy(outmsg->text, text);
            pthread_mutex_lock(outqueue->mtx);
            add(outqueue, outmsg);
            pthread_mutex_unlock(outqueue->mtx);

            memset(outmsg, '\0', textlen * sizeof(char));
            free(text);
        }
        pthread_mutex_unlock(&input_mtx);

        usleep(10000);
    }

exit:
    pthread_exit(1);
}
