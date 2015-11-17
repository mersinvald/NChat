#ifndef LIBCHAT_H
#define LIBCHAT_H
#include <time.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/un.h>

#define LC_MSG_TEXT_LEN 2048
#define LC_MSG_USERNAME_LEN 32

struct lc_socket_in_s {
    int    fd;
    ushort    slen;
    struct sockaddr_in saddr;
};

typedef struct lc_socket_in_s lc_sockin_t;

struct lc_socket_un_s {
    int    fd;
    ushort    slen;
    struct sockaddr_un saddr;
};

typedef struct lc_socket_un_s lc_sockun_t;

struct lc_message_s {
    char   text[LC_MSG_TEXT_LEN];
    char   username[LC_MSG_USERNAME_LEN];
    int    id;
    time_t out_time;
};

typedef struct lc_message_s lc_message_t;

#endif // LIBCHAT_H
