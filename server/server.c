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
#include <sig_handler.h>

#include "message_bay.h"
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

int parse_args(config* conf, int argc, char** argv){
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

int main(int argc, char** argv){
    int exit_code = ERR_NO;
    int n;

    /* Making server react on SIGTERM and SIGKILL */
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = lc_term;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGKILL, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    /* Setting up default config */
    config conf = {
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
    Socket_in listener;
    memset(&listener, 0, sizeof(Socket_in));

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

    /* Creating sockets for IPC */
    Socket_un socktrans;
    socktrans.fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(socktrans.fd < 0){
        lc_error("ERROR - socket(): can't create IPC socket\n%s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
        exit_code = ERR_CREATEIPCSOCK;
        goto close_listener;
    }
    lc_log_v(3, "Created IPC socket");

    socktrans.saddr.sun_family = AF_UNIX;
    socktrans.slen = sizeof(socktrans.saddr);

    /* Binding IPC socket */
    char buffer[128];
    int ipc_n = 0;
    lc_log_v(4, "Binding IPC socket");
    do {
        memset(&buffer, '\0', sizeof(buffer));
        sprintf(socktrans.saddr.sun_path, IPC_SOCKET_BINDPOINT, ipc_n++);
        n = bind(socktrans.fd, (struct sockaddr*)&socktrans.saddr, socktrans.slen);;
        if(n < 0) lc_log_v(4, "Bindpoint %s already reserved :(", socktrans.saddr.sun_path);
    } while(n < 0 && ipc_n < 100);
    if(n < 0){
        lc_error("ERROR - bind(): can't IPC socket\n%s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
        exit_code = ERR_BINDSOCK;
        goto close_ipc_socket;
    }
    lc_log_v(2, "Binded IPC socket to %s", socktrans.saddr.sun_path);

    /* Connecting process to IPC socket */
    if((connect(socktrans.fd, (struct sockaddr*)&socktrans.saddr, socktrans.slen)) < 0){
        lc_error("ERROR - connect(): can't connect to IPC socket\n%s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
        exit_code = ERR_CONNECT;
        goto close_ipc_socket;
    }
    lc_log_v(3, "Connected server process to IPC socket");

    /* Enabling non-block mode on IPC sockets */
    lc_log_v(4, "Switching IPC socket to non-blocking I/O mode");
    int flags;
    flags = fcntl(socktrans.fd, F_GETFL, 0);
    if(fcntl(socktrans.fd, F_SETFL, flags | O_NONBLOCK) < 0){
        lc_error("ERROR - fnctl(): can't switch socket to non-blocking I/O mode\n");
        exit_code = ERR_FCNTL;
        goto close_ipc_socket;
    }

    /* Creating message bay */
    int bay_pid;
    if((bay_pid = fork_bay(socktrans)) < 0){
        lc_error("ERROR - fork(): can't fork message bay subprocess\nProbably low memory or corruption");
        exit_code = ERR_CREATEBAY;
        goto close_ipc_socket;
    }
    lc_log_v(3, "Forked message bay subprocess");

    /* Listen for incoming connections */
    if((listen(listener.fd, SOMAXCONN)) < 0){
        lc_error("ERROR - listen(): can't start listening TCP connections\n%s", explain_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        exit_code = ERR_LISTEN;
        goto close_listener;
    }
    lc_log_v(1, "Listening on port %u", conf.port);

    /* Main listener loop */
    struct sockaddr_in newcliaddr;
    uint nclilen = sizeof(newcliaddr);
    int newsockfd;
    while(!lc_done){
        newsockfd = accept(listener.fd, (struct sockaddr*)&newcliaddr, &nclilen);
        if(newsockfd < 0){
            lc_error("ERROR - accept(): can't accept incoming connection\n%s", explain_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
            exit_code = ERR_ACCEPT;
            goto close_ipc_socket;
        }
        lc_log_v(2, "Reveived incoming connection from %s:%u", inet_ntoa(newcliaddr.sin_addr), ntohs(newcliaddr.sin_port));
        lc_log_v(4, "Created new socket for client %s:%u", inet_ntoa(newcliaddr.sin_addr), ntohs(newcliaddr.sin_port));

        lc_log_v(4, "Switching client socket to non-blocking I/O mode");
        flags = fcntl(newsockfd, F_GETFL, 0);
        if(fcntl(newsockfd, F_SETFL, flags | O_NONBLOCK) < 0){
            lc_error("ERROR - fnctl() error");
            exit_code = ERR_FCNTL;
            goto close_ipc_socket;
        }

        /* Sending connected client socket fd to bay */
        lc_log_v(4, "Sending new socket file descriptor to IPC with SCM_RIGHTS msghdr");
        n = ancil_send_fd(socktrans.fd, newsockfd);
        if(n < 0){
            lc_error("ERROR in main() - can't send ancillary fd");
            exit_code = ERR_SEND;
            goto close_ipc_socket;
        }

        usleep(100000);
    }

close_ipc_socket:
    lc_log_v(2, "Closing IPC socks");
    lc_log_v(3, "Closing socktrans socket");
    kill(bay_pid, SIGKILL);
    close(socktrans.fd);
close_listener:
    lc_log_v(2, "Closing listener socket");
    close(listener.fd);
close_files:
    if(LC_LOG_TO_FILE) fclose(logfile);
exit:
    lc_log_v(1, "Exiting with code %i", exit_code);
    return exit_code;
}

