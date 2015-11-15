#ifndef SERVER_H
#define SERVER_H
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>

struct config_s {
    ushort port;
    ushort verb;
    char*  logfile;
    char*  name;
};

typedef struct config_s config;

#endif // SERVER_H

