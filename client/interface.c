#include "interface.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct input_thread_data_s {
    char* buffer;
    pthread_mutex_t* mtx;
} input_tdata;

void* input(void* arg){
    input_tdata* data = (input_tdata*) arg;
    while(1){
        pthread_mutex_lock(data->mtx);
        if(fgets(data->buffer, LC_MSG_TEXT_LEN, stdin) == NULL){
            pthread_mutex_unlock(data->mtx);
            return -1;
        }
        pthread_mutex_unlock(data->mtx);
    }
}

void* interface(void* arg){
    interface_tdata* data = (interface_tdata*) arg;
    msgqueue *inqueue = data->inqueue;
    msgqueue *outqueue = data->outqueue;


    /* Setting up input thread */
    char* input_buffer = calloc(LC_MSG_TEXT_LEN * sizeof(char), '\0');
    pthread_mutex_t input_mtx = PTHREAD_MUTEX_INITIALIZER;
    input_tdata input_args = {
        input_buffer,
        &input_mtx
    };

    pthread_t input_thread;
    pthread_create(&input_thread, NULL, &input, &input_args);



    message *inmsg, *outmsg;
    outmsg = calloc(sizeof(message), '\0');
    while(1){
        pthread_mutex_lock(&inqueue->mtx);
        if(inqueue->lenght > 0){
            inmsg = lc_msgque_pop(inqueue);
            if(inmsg == NULL)
                goto exit;
            pthread_mutex_unlock(&inqueue->mtx);
            printf("%s: %s\n", inmsg->username, inmsg->text);
            free(inmsg);
        }

        pthread_mutex_lock(&input_mtx);
        if(input_buffer[0] != '\0'){
            int textlen = strlen(input_buffer);
            strcpy(outmsg->text, input_buffer);
            pthread_mutex_lock(&outqueue->mtx);
            lc_msgque_queue(outqueue, outmsg);
            pthread_mutex_unlock(&outqueue->mtx);
            memset(input_buffer, '\0', textlen * sizeof(char));
            memset(outmsg, '\0', textlen * sizeof(char));
        }
        pthread_mutex_unlock(&input_mtx);

        usleep(10000);
    }

exit:
    pthread_exit(1);
}
