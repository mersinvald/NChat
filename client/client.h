#ifndef CLIENT_H
#define CLIENT_H
#include <stdlib.h>
#include <types.h>

#define DEFAULT_PORT     30200
#define DEFAULT_VERB     1
#define DEFAULT_LOGFILE  "./chat_client.log"
#define DEFAULT_USERNAME "Anonimous"
#define DEFAULT_SHOST    NULL
#define DEFAULT_SIP      NULL
#define DEFAULT_SPORT    -1

struct config_s {
    ushort port;
    ushort verb;

    char* serv_hostname;
    char* serv_ip;
    ushort serv_port;

    char* logfile;
    char* username;
};

typedef struct config_s config;

config conf;

extern int init_msg(lc_message_t* msg);

#endif // CLIENT_H

