#include "sig_handler.h"
#include "log.h"

#include <signal.h>
#include <strings.h>
#include <memory.h>

volatile int lc_done = 0;

int lc_term(int signum){
    char sig[16];
    switch(signum){
    case SIGINT:  strcpy(sig, "SIGINT");  break;
    case SIGTERM: strcpy(sig, "SIGTERM"); break;
    case SIGKILL: strcpy(sig, "SIGKILL"); break;
    default:  return -1;
    }
    lc_log_v(1, "Got %s, shutting down.", sig);
    lc_done = 1;
    return 0;
}


