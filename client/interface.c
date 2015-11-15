#include "interface.h"
#include "client.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void* input(void* arg){
    lc_queue_t* in_q = (lc_queue_t*) arg;
    char buffer[LC_MSG_TEXT_LEN];
    while(1){
        if(fgets(buffer, LC_MSG_TEXT_LEN, stdin) != NULL){
            pthread_mutex_lock(in_q->mtx);
            lc_queue_add(in_q, &buffer);
            memset(buffer, '\0', strlen(buffer));
            pthread_mutex_unlock(in_q->mtx);
        }
    }
}

void* interface(void* arg){
    interface_tdata_t* data = (interface_tdata_t*) arg;
    lc_queue_t *inqueue = data->inqueue;
    lc_queue_t *outqueue = data->outqueue;


    /* Setting up input thread */
    pthread_mutex_t input_mtx = PTHREAD_MUTEX_INITIALIZER;
    lc_queue_t input_queue;
    input_queue.lenght = 0;
    input_queue.mtx = &input_mtx;
    input_queue.ssize = LC_MSG_TEXT_LEN;

    pthread_t input_thread;
    pthread_create(&input_thread, NULL, &input, &input_queue);

    /* Interface loop */
    lc_message_t *inmsg, *outmsg;
    outmsg = malloc(sizeof(lc_message_t));
    while(1){
        /* Check if there are incoming messages */
        pthread_mutex_lock(inqueue->mtx);
        if(inqueue->lenght > 0){
            inmsg = (lc_message_t*) lc_queue_pop(inqueue);
            if(inmsg == NULL)
                goto exit;
            pthread_mutex_unlock(inqueue->mtx);
            printf("%s: %s", inmsg->username, inmsg->text);
            free(inmsg);
        }
        pthread_mutex_unlock(inqueue->mtx);

        /* Check if there is user input */
        pthread_mutex_lock(&input_mtx);
        if(input_queue.lenght > 0){
            char* text = (char*) lc_queue_pop(&input_queue);
            int textlen = strlen(text);

            init_msg(outmsg);

            strcpy(outmsg->text, text);
            pthread_mutex_lock(outqueue->mtx);
            lc_queue_add(outqueue, outmsg);
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
