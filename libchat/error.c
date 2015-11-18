#include "error.h"
#include <stdio.h>
#include <limits.h>
#include <arpa/inet.h>

#define MIN_NONSYSTEM_PORT 2000

int lc_is_port_valid(int port){
    return (port > MIN_NONSYSTEM_PORT && port <= USHRT_MAX);
}

int lc_is_ip_valid(int domain, const char* ip){
    struct sockaddr dummy;
    return (inet_pton(domain, ip, &dummy) <= 0) ? 0 : 1;
}
