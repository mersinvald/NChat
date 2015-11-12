#include "client.h"
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include <libexplain/socket.h>
#include <libexplain/gethostbyname.h>
#include <libexplain/fcntl.h>
#include <libexplain/fopen.h>

#include <log.h>
#include <types.h>
#include <error.h>
#include <non_block_io.h>

#define BUFFSIZE 512


void usage(char* arg0){
    printf("Usage: %s [server IP or host] [port] [options]\n\n"
           "Options:\n"
           "-u STRING [\"%s\"] -- username\n"
           "-l path_to_file [%s]-- logfile\n"
           "-v [%i]\t\t -- verbosity level. (-vvv means verbosity level 3)\n",
           arg0, DEFAULT_LOGFILE, DEFAULT_USERNAME, DEFAULT_VERB);
}

int parse_args(config* conf, int argc, char** argv){
    int n, i;
    int verb = 0;
    int hostflag = 0, ipflag = 0, portflag = 0;
    while((n = getopt(argc, argv, "p:u:l:v")) != -1){
        switch(n){
        case 'u': conf->username = optarg; break;
        case 'l': conf->logfile = optarg; break;
        case 'v': verb++; break;
        default: return -1; break;
        }
    }

    for(i = 1; i < argc; i++){
        if(!strcmp("-v", argv[i])) continue;
        if(!strcmp("-p", argv[i]) || !strcmp("-u", argv[i]) || !strcmp("-l", argv[i])) {
            i++;
            continue;
        }
        if(lc_is_port_valid(atoi(argv[i])) && !portflag) {
            conf->serv_port = atoi(argv[i]);
            portflag = 1;
            continue;
        }
        else
        if(lc_is_ip_valid(AF_INET, argv[i]) && !hostflag && !ipflag){
            conf->serv_ip = argv[i];
            ipflag = 1;
            continue;
        }
        else
        if(!hostflag && !ipflag){
            conf->serv_hostname = argv[i];
            hostflag = 1;
            continue;
        }
        return -1;
    }

    conf->verb = (verb > DEFAULT_VERB) ? verb : DEFAULT_VERB;
    return 0;
}

int check_conf(config* conf){
    return (conf->serv_hostname != NULL || conf->serv_ip != NULL) && (conf->serv_port > 0);
}

int init_msg(message *msg, config *conf){
    //@TODO memset error handling
    memset(msg, '\0', sizeof(message));
    strcpy(msg->username, conf->username);
    return 0;
}

int main(int argc, char** argv){
    int exit_code = ERR_NO;
    int n, i;

    /* Setting up default config */
    config conf = {
        DEFAULT_PORT,
        DEFAULT_VERB,
        DEFAULT_SHOST,
        DEFAULT_SIP,
        DEFAULT_SPORT,
        DEFAULT_LOGFILE,
        DEFAULT_USERNAME
    };

    if(parse_args(&conf, argc, argv) < 0 || !check_conf(&conf)){
        usage(argv[0]);
        exit_code = ERR_PARSEARGS;
        goto exit;
    }

    /* Setting verbosity level */
    lc_set_verbosity(conf.verb);

    /* Rerouting log to file */
    FILE* logfile;
    if(conf.logfile != NULL){
        if((logfile = fopen(conf.logfile, "w+")) == NULL)
            lc_error("ERROR - fopen(): can't open file %s\n%s", conf.logfile, explain_fopen(conf.logfile, "w+"));
        else
            lc_log_to_file(logfile);
    }

    /* Involving interface in separate thread */
    /* @TODO ncurses TUI */

    /* Client thread */

    /* Resolving IP address if hostneme was passed */
    if(conf.serv_hostname != NULL){
        struct hostent *he;
        struct in_addr **addr_list;

        if((he = gethostbyname(conf.serv_hostname)) == NULL){
            lc_error("ERROR - gethostbyname: can't resolve server's ip\n%s", explain_gethostbyname(conf.serv_hostname));
            exit_code = ERR_RESOLVE;
            goto close_logfile;
        }

        conf.serv_ip = malloc(3*4*sizeof(char));
        addr_list = (struct in_addr **) he->h_addr_list;
        for(i = 0; addr_list[i] != NULL; i++){
            strcpy(conf.serv_ip, inet_ntoa(*addr_list[i]));
            break;
        }
    }

    /* Setting up client */
    int clifd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(clifd < 0){
        lc_error("ERROR - socket(): can't create client TCP socket\n%s", explain_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        exit_code = ERR_CREATESOCK;
        goto close_logfile;
    }

    /* Setting up server struct */
    struct sockaddr_in saddr;
    uint saddrlen;
    memset(&saddr, 0, sizeof(saddr));

    saddr.sin_addr.s_addr = inet_addr(conf.serv_ip);
    saddr.sin_family = AF_INET;
    saddr.sin_port   = htons(conf.serv_port);
    saddrlen = sizeof(saddr);

    /* Connecting to server */
    if(connect(clifd, (struct sockaddr*)&saddr, saddrlen) < 0){
        lc_error("ERROR - connect(): couldn't connect to server %s:%u(%u)\n%s", conf.serv_ip, conf.serv_port, ntohl(saddr.sin_addr.s_addr), explain_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        exit_code = ERR_CONNECT;
        goto close_client;
    }

    /* Switching socket to non-blocking I/O mode */
    int flags = fcntl(clifd, F_GETFL, 0);
    if(fcntl(clifd, F_SETFL, flags | O_NONBLOCK) < 0){
        lc_error("ERROR - fcntl(): error\n%s", explain_fcntl(clifd, F_SETFL, flags | O_NONBLOCK));
        exit_code = ERR_FCNTL;
        goto close_client;
    }

    message msg;
    char buffer[sizeof(msg.text)];
    while(1){
        init_msg(&msg, &conf);

        n = lc_recv_non_block(clifd, &msg, sizeof(message), 0);
        if(n < 0){
            exit_code = ERR_RECV;
            goto close_client;
        }
        if(n > 0){
            printf("%s: %s", msg.username, msg.text);
        }

        if(fgets(buffer, sizeof(buffer), stdin) != NULL){
            strcpy(msg.text, buffer);
            n = lc_send_non_block(clifd, &msg, sizeof(msg), 0);
            if(n < 0){
                exit_code = ERR_SEND;
                goto close_client;
            }
        }
    }



close_client:
    shutdown(clifd, SHUT_RDWR);
    close(clifd);
close_logfile:
exit:
    lc_log_v(1, "Exititng with code %i", exit_code);
    if(LC_LOG_TO_FILE) fclose(logfile);
    return exit_code;
}

