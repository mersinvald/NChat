#ifndef SERVER_H
#define SERVER_H
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>

/* Server configuration */
struct config_s {
    ushort port;
    ushort verb;
    char*  logfile;
    char*  name;
};

typedef struct config_s config_t;

#endif // SERVER_H
