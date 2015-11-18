#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <memory.h>

#include <fcntl.h>
#include <errno.h>
#include <ancillary.h>
#include <signal.h>

#include <libexplain/socket.h>
#include <libexplain/fcntl.h>
#include <arpa/inet.h>

#include <log.h>
#include <error.h>
#include <non_block_io.h>

#include "queue.h"
#include "message_relay.h"
#include "server.h"

#define DEFAULT_PORT 8000
#define DEFAULT_VERB 1
#define DEFAULT_LOG  NULL
#define DEFAULT_NAME "Chat"
#define IPC_SOCKET_BINDPOINT "/tmp/chat_ipc-%i.sock"

void usage(char* arg0){
    printf("Usage: %s [options]\n\n"
           "Options:\n"
           "-p NUM [%i]\t -- server port, must be in range %i-%i\n"
           "-l path_to_file  -- logfile, by default logging to stdout/stderr\n"
           "-v [%i]\t\t -- verbosity level. (-vvv means verbosity level 3)\n"
           "-n string [Chat] \t -- chat name that will be displayed to clients\n",
           arg0, DEFAULT_PORT, 2000, USHRT_MAX, DEFAULT_VERB);
}

int parse_args(config_t* conf, int argc, char** argv){
    int n;
    int verb = 0;
    while((n = getopt(argc, argv, "p:l:n:v")) != -1){
        switch(n){
        case 'p':
            if(lc_is_port_valid(atoi(optarg)))
                conf->port = atoi(optarg);
            else return -1;
            break;
        case 'l': conf->logfile = optarg; break;
        case 'v': verb++; break;
        case 'n': conf->name = optarg; break;
        default: return -1; break;
        }
    }
    conf->verb = (verb > DEFAULT_VERB) ? verb : DEFAULT_VERB;
    return 0;
}

volatile int listener_done = 0;
void listener_term_handler(int signum){
    char sig[16];
    switch(signum){
    case SIGINT:  strcpy(sig, "SIGINT");  break;
    case SIGTERM: strcpy(sig, "SIGTERM"); break;
    case SIGKILL: strcpy(sig, "SIGKILL"); break;
    default: sprintf(sig, "SIGNALL %i", signum);
    }
    lc_log_v(1, "Got %s, shutting down.", sig);
    listener_done = 1;
}

int main(int argc, char** argv){
    int exit_code = ERR_NO;

    /* Making server react on SIGTERM and SIGKILL */
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = listener_term_handler;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGKILL, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    /* Setting up default config */
    config_t conf = {
        DEFAULT_PORT,
        DEFAULT_VERB,
        DEFAULT_LOG,
        DEFAULT_NAME
    };

    /* Parsing command-line arguments */
    if(parse_args(&conf, argc, argv) < 0){
        usage(argv[0]);
        exit_code = ERR_PARSEARGS;
        goto exit;
    }

    lc_set_verbosity(conf.verb);

    /* Rerouting loging to file if specified */
    FILE* logfile;
    if(conf.logfile != NULL){
        if((logfile = fopen(conf.logfile, "w+")) == NULL)
            lc_error("ERROR - fopen(): can't open file %s, falling back to stdout logging", conf.logfile);
        else
            lc_log_to_file(logfile);
    }

    lc_log_v(1, "Starting chat server");
    lc_log_v(3, "Chat Server parameters: "
                "port %u, logging to %s, verbosity level %u",
             conf.port, (conf.logfile != NULL) ? conf.logfile : "stdout/stderr", conf.verb);

    /* Initializing listening socket */
    lc_sockin_t listener;
    memset(&listener, 0, sizeof(lc_sockin_t));

    listener.saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    listener.saddr.sin_family = AF_INET;
    listener.saddr.sin_port = htons(conf.port);
    listener.slen = sizeof(listener.saddr);

    if((listener.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        lc_error("ERROR - socket(): can't create TCP socket\n%s", explain_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        exit_code = ERR_CREATESOCK;
        goto close_files;
    }
    lc_log_v(2, "Created TCP listener socket");

    /* Bind socket to port */
    if((bind(listener.fd, (struct sockaddr*)&listener.saddr, listener.slen)) < 0){
        lc_error("ERROR - bind(): can't bind socket to port %i\n%s", conf.port, explain_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        exit_code = ERR_BINDSOCK;
        goto close_listener;
    }
    lc_log_v(2, "Binded listener socket to port %i", conf.port);

    /* Creating message relay */
    pthread_t relay_thr;
    pthread_mutex_t relay_mtx = PTHREAD_MUTEX_INITIALIZER;
    lc_queue_t* relay_q = malloc(sizeof(lc_queue_t));
    relay_q->ssize = sizeof(int);
    relay_q->lenght = 0;
    relay_q->mtx = &relay_mtx;

    if((pthread_create(&relay_thr, NULL, relay_thread, relay_q)) < 0){
        lc_error("ERROR - pthread_create(): can't create relay thread\nProbably low memory or corruption");
        exit_code = ERR_PTHREAD;
        goto close_listener;
    }
    lc_log_v(3, "Intitalized message relay thread");

    /* Listen for incoming connections */
    if((listen(listener.fd, SOMAXCONN)) < 0){
        lc_error("ERROR - listen(): can't start listening TCP connections\n%s", explain_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        exit_code = ERR_LISTEN;
        goto kill_relay;
    }
    lc_log_v(1, "Listening on port %u", conf.port);

    /* Main listener loop */
    struct sockaddr_in newcliaddr;
    uint nclilen = sizeof(newcliaddr);
    int newclientfd;
    int flags;
    while(!listener_done){
        newclientfd = accept(listener.fd, (struct sockaddr*)&newcliaddr, &nclilen);
        if(newclientfd < 0){
            lc_error("ERROR - accept(): can't accept incoming connection\n%s", explain_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
            exit_code = ERR_ACCEPT;
            goto kill_relay;
        }
        lc_log_v(2, "Reveived incoming connection from %s:%u", inet_ntoa(newcliaddr.sin_addr), ntohs(newcliaddr.sin_port));
        lc_log_v(4, "Created new socket for client %s:%u", inet_ntoa(newcliaddr.sin_addr), ntohs(newcliaddr.sin_port));

        lc_log_v(4, "Switching client socket to non-blocking I/O mode");
        flags = fcntl(newclientfd, F_GETFL, 0);
        if(fcntl(newclientfd, F_SETFL, flags | O_NONBLOCK) < 0){
            lc_error("ERROR - fnctl() error");
            exit_code = ERR_FCNTL;
            goto kill_relay;
        }

        pthread_mutex_lock(&relay_mtx);
        if(lc_queue_add(relay_q, &newclientfd) < 0){
            lc_error("ERROR - lc_queue_add(): can't add new fd to queue\nProbably low memory or corruption");
            goto kill_relay;
        }
        pthread_mutex_unlock(&relay_mtx);

        usleep(100000);
    }

kill_relay:
    lc_log_v(3, "Stopping relay thread");
    pthread_kill(relay_thr, SIGTERM);
close_listener:
    lc_log_v(2, "Closing listener socket");
    shutdown(listener.fd, SHUT_RDWR);
    close(listener.fd);
close_files:
    if(LC_LOG_TO_FILE) fclose(logfile);
exit:
    lc_log_v(1, "Exiting with code %i", exit_code);
    return exit_code;
}
