#include "error.h"
#include <stdio.h>
#include <limits.h>
#include <arpa/inet.h>

int lc_is_port_valid(int port){
    return (port > 2000 && port <= USHRT_MAX);
}

int lc_is_ip_valid(int domain, char *ip){
    return (inet_pton(domain, ip, NULL) <= 0) ? 0 : 1;
}
