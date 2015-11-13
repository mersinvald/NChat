#include "message_bay.h"

#include <sys/ioctl.h>
#include <signal.h>

#include <types.h>
#include <log.h>
#include <non_block_io.h>
#include <sig_handler.h>
#include <ancillary.h>

#include <libexplain/socket.h>
#include <errno.h>

#include <memory.h>
#include <unistd.h>

int add_client(struct bay_s *bay, int fd){
    int* temp = realloc(bay->clientsfd, ++bay->count);
    if(temp == NULL) return -1;

    bay->clientsfd = temp;
    bay->clientsfd[bay->count-1] = fd;
    return 0;
}

int del_client(struct bay_s *bay, int index){
    shutdown(bay->clientsfd[index], SHUT_RDWR);
    close(bay->clientsfd[index]);
    int* s = bay->clientsfd+(index * sizeof(int));
    if(bay->count > 1) {
        int* temp = memmove(s, s+sizeof(int), (bay->count-index) * sizeof(int));
        if(temp == NULL) return -1;
        bay->count--;
    } else {
        free(bay->clientsfd);
         bay->count = 0;
    }
    return 0;
}

void broadcast(struct bay_s *bay, message* msg, int size){
    int i, fd, n;
    for(i = 0; i < bay->count; i++){
        fd = bay->clientsfd[i];
        n = lc_send_non_block(fd, msg, size, 0);
        if(n < 0){
            lc_error("ERROR - lc_send_non_block(): got -1, deleting this client");
            del_client(bay, i);
        }
     }
}

int fork_bay(Socket_un socktrans){
    int pid = fork();
    if(pid < 0)        // fork() failed
        return -1;
    if(pid > 0)         // it's parent process -> return child's pid
        return pid;

    /* Now we are in child process */

    /* React on SIGTERM and SIGKILL */
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = lc_term;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGKILL, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    struct bay_s bay;
    bay.clientsfd = NULL;
    bay.count = 0;

    int n, i;
    message chatmsg;
    int     newclientfd;

    signal(SIGPIPE, SIGINT);

    while(!lc_done){
        memset(&chatmsg, 0, sizeof(chatmsg));
        n = ancil_recv_fd(socktrans.fd, &newclientfd);
        if(n < 0){ // seems like bug: getting -1 on succsess
            if(errno != EAGAIN && errno != EINTR){
                lc_error("ERROR - ancil_recv_fd(): couldn't receive ancillary fd\n%s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
                goto exit;
            }
        } else
        if(add_client(&bay, newclientfd) < 0){
            lc_error("ERROR - add_client(): can't add new client, realloc() failure\nProbably low memory or corruption");
            goto exit;
        }

        for(i = 0; i < bay.count; i++){
            int fd = bay.clientsfd[i];
            n = lc_recv_non_block(fd, &chatmsg, sizeof(chatmsg), 0);
            if(n < 0){
                lc_error("ERROR - lc_recv_non_block(): returned -1, deleting this client");
                del_client(&bay, i);
            }
            if(n > 0){
                lc_log_v(4, "Received message from one of clients --> broadcasting...");
                broadcast(&bay, &chatmsg, sizeof(chatmsg));
            }
        }

       usleep(100000);
    }

exit:
    lc_log_v(3, "Freeing bay");
    free(bay.clientsfd);
    close(socktrans.fd);
    exit(0);
}
