#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

FILE *lc_outfile;
int  LC_LOG_TO_FILE;
int  LC_VERB;

extern void lc_log_v(int verblevel, const char* message, ...);
extern void lc_log(const char* message, ...);
extern void lc_error(const char* message, ...);
extern void lc_log_to_file(FILE *f);
extern void lc_set_verbosity(int v);
#endif // LOG_H
