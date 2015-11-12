#ifndef LIBCHAT_H
#define LIBCHAT_H
#include <time.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/un.h>

struct socket_in_s {
    int    fd;
    uint    slen;
    struct sockaddr_in saddr;
};

typedef struct socket_in_s Socket_in;

struct socket_un_s {
    int    fd;
    uint    slen;
    struct sockaddr_un saddr;
};

typedef struct socket_un_s Socket_un;

struct message_s {
    char   text[2048];
    char   username[32];
    int    id;
    time_t out_time;
};

typedef struct message_s message;

#endif // LIBCHAT_H
