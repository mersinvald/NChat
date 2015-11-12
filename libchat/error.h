#ifndef ERROR_H
#define ERROR_H

enum {
    ERR_NO = 0,
    ERR_PARSEARGS,
    ERR_CREATESOCK,
    ERR_BINDSOCK,
    ERR_CONNECT,
    ERR_LISTEN,
    ERR_ACCEPT,
    ERR_CREATEIPCSOCK,
    ERR_CREATEBAY,
    ERR_FCNTL,
    ERR_FORK,
    ERR_SEND,
    ERR_RECV,
    ERR_RESOLVE
};

extern int lc_is_port_valid(int port);
extern int lc_is_ip_valid(int domain, char* ip);

#endif // ERROR_H

